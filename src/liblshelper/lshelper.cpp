#include "lshelper.h"
#include "lightlyshaders_config.h"

#include <effect/effecthandler.h>
#include <window.h>

#include <QBitmap>
#include <QPainter>
#include <QRegion>
#include <QRegularExpression>
#include <QStringList>
#include <QtMath>

#include <algorithm>

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
        m_size = static_cast<int>(m_size * 0.5 * m_squircleRatio);
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

    // Indexed by TopLeft, TopRight, BottomRight, BottomLeft: where each corner
    // is in the mask image, and where it goes relative to its window corner
    const QPoint origins[NCorners] = {
        QPoint(0, 0),
        QPoint(size, 0),
        QPoint(size, size),
        QPoint(0, size),
    };
    const QPoint offsets[NCorners] = {
        QPoint(-m_shadowOffset + 1, -m_shadowOffset + 1),
        QPoint(-m_size - 1, -m_shadowOffset + 1),
        QPoint(-m_size - 1, -m_size - 1),
        QPoint(-m_shadowOffset + 1, -m_size - 1),
    };

    for (int corner = 0; corner < NCorners; ++corner) {
        const QImage cornerImg = img.copy(QRect(origins[corner], QSize(size, size)))
                                     .createMaskFromColor(QColor(Qt::black).rgb(), Qt::MaskOutColor);
        const QRegion mask(QBitmap::fromImage(cornerImg, Qt::DiffuseAlphaDither));
        m_cornerMasks[corner] = RegionF(mask.translated(offsets[corner]));
    }
}

void
LSHelper::roundBlurRegion(EffectWindow *w, RegionF *blur_region) const
{
    if(blur_region->isEmpty() || !shouldRound(w)) {
        return;
    }

    // The blur region is relative to the window's top left corner
    const int width = static_cast<int>(w->frameGeometry().width());
    const int height = static_cast<int>(w->frameGeometry().height());

    *blur_region -= m_cornerMasks[TopLeft];
    *blur_region -= m_cornerMasks[TopRight].translated(width, 0);
    *blur_region -= m_cornerMasks[BottomRight].translated(width, height);
    *blur_region -= m_cornerMasks[BottomLeft].translated(0, height);
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
    // Shells, docks and launchers that draw their own shape, matched by window class
    // when they don't have a server side decoration
    static const QStringList undecoratedExclusions = {
        QStringLiteral("plasma"),
        QStringLiteral("krunner"),
        QStringLiteral("sddm"),
        QStringLiteral("vmware-user"),
        QStringLiteral("latte-dock"),
        QStringLiteral("lattedock"),
        QStringLiteral("plank"),
        QStringLiteral("cairo-dock"),
        QStringLiteral("albert"),
        QStringLiteral("ulauncher"),
        QStringLiteral("ksplash"),
        QStringLiteral("ksmserver"),
    };

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

    const QString windowClass = w->windowClass();
    const auto classContains = [&windowClass](const QString &name) {
        return windowClass.contains(name, Qt::CaseInsensitive);
    };

    if (!w->hasDecoration()) {
        if (std::any_of(undecoratedExclusions.cbegin(), undecoratedExclusions.cend(), classContains)) {
            return false;
        }
        if (classContains(QStringLiteral("reaper")) && !hasShadow(w)) {
            return false;
        }
    }

    if (classContains(QStringLiteral("xwaylandvideobridge")))
        return false;

    // JetBrains IDEs use normal windows captioned "win<N>" for their popups
    static const QRegularExpression jetbrainsPopup(QStringLiteral("win[0-9]+"));
    if (classContains(QStringLiteral("jetbrains")) && w->caption().contains(jetbrainsPopup))
        return false;

    if (classContains(QStringLiteral("plasma")) && !w->isNormalWindow() && !w->isDialog() && !w->isModal())
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
