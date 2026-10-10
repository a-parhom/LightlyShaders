#include "lshelper.h"
#include "lightlyshaders_config.h"

#include <effect/effecthandler.h>
#include <window.h>

#include <QBitmap>
#include <QPainter>
#include <QRegularExpression>
#include <QtMath>

namespace KWin {

static int signum(float val)
{
    return (0.0f < val) - (val < 0.0f);
}

static bool isMaximized(EffectWindow *w)
{
    return w->window()->maximizeMode() == MaximizeFull;
}

void
LSHelper::reconfigure()
{
    LightlyShadersConfig::self()->load();

    m_cornersType = LightlyShadersConfig::cornersType();
    m_squircleRatio = LightlyShadersConfig::squircleRatio();
    m_shadowOffset = LightlyShadersConfig::shadowOffset();
    m_size = LightlyShadersConfig::roundness();
    m_disabledForMaximized = LightlyShadersConfig::disabledForMaximized();

    if(m_cornersType == SquircledCorners) {
        m_size = m_size * 0.5 * m_squircleRatio;
    }

    // The shadow sampling offset has to stay inside the corner
    m_shadowOffset = std::clamp(m_shadowOffset, 0, std::max(m_size - 1, 0));

    setMaskRegions();
}

void
LSHelper::setMaskRegions()
{
    const int size = m_size + m_shadowOffset;
    const QImage img = genMaskImg(size);

    // Indexed by TopLeft, TopRight, BottomRight, BottomLeft
    const QPoint origins[NCorners] = {
        QPoint(0, 0),
        QPoint(size, 0),
        QPoint(size, size),
        QPoint(0, size),
    };

    for (int corner = 0; corner < NCorners; ++corner) {
        const QImage cornerImg = img.copy(QRect(origins[corner], QSize(size, size)))
                                     .createMaskFromColor(QColor(Qt::black).rgb(), Qt::MaskOutColor);
        m_maskRegions[corner] = QRegion(QBitmap::fromImage(cornerImg, Qt::DiffuseAlphaDither));
    }
}

void
LSHelper::roundBlurRegion(EffectWindow *w, RegionF *blur_region) const
{
    if(blur_region->isEmpty() || !shouldRound(w)) {
        return;
    }

    const QRectF geo(w->frameGeometry());

    QRegion top_left = m_maskRegions[TopLeft];
    top_left.translate(0-m_shadowOffset+1, 0-m_shadowOffset+1);
    *blur_region = blur_region->subtracted(RegionF(top_left));

    QRegion top_right = m_maskRegions[TopRight];
    top_right.translate(geo.width() - m_size-1, 0-m_shadowOffset+1);
    *blur_region = blur_region->subtracted(RegionF(top_right));

    QRegion bottom_right = m_maskRegions[BottomRight];
    bottom_right.translate(geo.width() - m_size-1, geo.height()-m_size-1);
    *blur_region = blur_region->subtracted(RegionF(bottom_right));

    QRegion bottom_left = m_maskRegions[BottomLeft];
    bottom_left.translate(0-m_shadowOffset+1, geo.height()-m_size-1);
    *blur_region = blur_region->subtracted(RegionF(bottom_left));
}

QPainterPath
LSHelper::superellipse(float size, int n, int translate) const
{
    const float n2 = 2.0 / n;
    const int steps = 360;
    const float step = (2 * M_PI) / steps;

    QPainterPath path;
    path.moveTo(2*size, size);

    for (int i = 1; i < steps; ++i)
    {
        const float t = i * step;
        const float cosT = qCos(t);
        const float sinT = qSin(t);

        const float x = size + (qPow(qAbs(cosT), n2) * size * signum(cosT));
        const float y = size - (qPow(qAbs(sinT), n2) * size * signum(sinT));

        path.lineTo(x, y);
    }
    path.lineTo(2*size, size);

    path.translate(translate,translate);

    return path;
}

// Black everywhere except the rounded shape, which is cut out. The black
// parts of each quarter are what gets removed from the blur region.
QImage
LSHelper::genMaskImg(int size) const
{
    QImage img(size*2, size*2, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.fillRect(img.rect(), Qt::black);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.setRenderHint(QPainter::Antialiasing);
    if (m_cornersType == SquircledCorners) {
        const QPainterPath squircle = superellipse((size-m_shadowOffset), m_squircleRatio, m_shadowOffset);
        p.drawPolygon(squircle.toFillPolygon());
    } else {
        p.drawEllipse(img.rect().adjusted(m_shadowOffset,m_shadowOffset,-m_shadowOffset,-m_shadowOffset));
    }
    p.end();

    return img;
}

bool
LSHelper::hasShadow(EffectWindow *w)
{
    return w->expandedGeometry().size() != w->frameGeometry().size();
}

bool
LSHelper::isManagedWindow(EffectWindow *w)
{
    if (w->isDesktop()
            || w->isPopupMenu()
            || w->isTooltip()
            || w->isSpecialWindow()
            || w->isDropdownMenu()
            || w->isPopupWindow()
            || w->isLockScreen()
            || w->isSplash()
            || w->isOnScreenDisplay()
            || w->isUtility()
            || w->isDock()
            || w->isToolbar()
            || w->isMenu())
        return false;

    if (
            (!w->hasDecoration()
                && (
                    w->windowClass().contains("plasma", Qt::CaseInsensitive)
                    || w->windowClass().contains("krunner", Qt::CaseInsensitive)
                    || w->windowClass().contains("sddm", Qt::CaseInsensitive)
                    || w->windowClass().contains("vmware-user", Qt::CaseInsensitive)
                    || w->windowClass().contains("latte-dock", Qt::CaseInsensitive)
                    || w->windowClass().contains("lattedock", Qt::CaseInsensitive)
                    || w->windowClass().contains("plank", Qt::CaseInsensitive)
                    || w->windowClass().contains("cairo-dock", Qt::CaseInsensitive)
                    || w->windowClass().contains("albert", Qt::CaseInsensitive)
                    || w->windowClass().contains("ulauncher", Qt::CaseInsensitive)
                    || w->windowClass().contains("ksplash", Qt::CaseInsensitive)
                    || w->windowClass().contains("ksmserver", Qt::CaseInsensitive)
                    || (w->windowClass().contains("reaper", Qt::CaseInsensitive) && !hasShadow(w))
                )
            )
            || w->windowClass().contains("xwaylandvideobridge", Qt::CaseInsensitive)

        ) return false;

    static const QRegularExpression jetbrainsPopup(QStringLiteral("win[0-9]+"));
    if(w->windowClass().contains("jetbrains", Qt::CaseInsensitive) && w->caption().contains(jetbrainsPopup))
        return false;

    if (w->windowClass().contains("plasma", Qt::CaseInsensitive) && !w->isNormalWindow() && !w->isDialog() && !w->isModal())
        return false;

    return true;
}

void
LSHelper::windowAdded(EffectWindow *w)
{
    if(isManagedWindow(w)) {
        m_managed.insert(w);
    }
}

void
LSHelper::windowDeleted(EffectWindow *w)
{
    m_managed.remove(w);
}

bool
LSHelper::isManaged(EffectWindow *w) const
{
    return m_managed.contains(w);
}

bool
LSHelper::shouldRound(EffectWindow *w) const
{
    return isManaged(w)
        && !w->isFullScreen()
        && !(m_disabledForMaximized && isMaximized(w));
}

} // namespace KWin
