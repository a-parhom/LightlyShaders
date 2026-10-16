#include "lshelper.h"
#include "lightlyshaders_config.h"

#include <QBitmap>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

Q_LOGGING_CATEGORY(LSHELPER, "liblshelper", QtWarningMsg)

namespace KWin {

LSHelper::LSHelper() : QObject()
{
    for (int i = 0; i < NTex; ++i)
    {
        m_maskRegions[i] = 0;
    }
}

LSHelper::~LSHelper()
{
    //delete mask regions
    for (int i = 0; i < NTex; ++i)
    {
        if (m_maskRegions[i])
            delete m_maskRegions[i];
    }

    m_managed.clear();
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

    setMaskRegions();
}

int
LSHelper::roundness()
{
    return m_size;
}

void
LSHelper::setMaskRegions()
{
    int size = m_size + m_shadowOffset;
    QImage img = genMaskImg(size, true, false);

    m_maskRegions[TopLeft] = createMaskRegion(img, size, TopLeft);
    m_maskRegions[TopRight] = createMaskRegion(img, size, TopRight);
    m_maskRegions[BottomRight] = createMaskRegion(img, size, BottomRight);
    m_maskRegions[BottomLeft] = createMaskRegion(img, size, BottomLeft);
}

Region *
LSHelper::createMaskRegion(QImage img, int size, int corner)
{
    QImage img_copy;

    switch(corner) {
        case TopLeft:
            img_copy = img.copy(0, 0, size, size);
            break;
        case TopRight:
            img_copy = img.copy(size, 0, size, size);
            break;
        case BottomRight:
            img_copy = img.copy(size, size, size, size);
            break;
        case BottomLeft:
            img_copy = img.copy(0, size, size, size);
            break;
    }

    img_copy = img_copy.createMaskFromColor(QColor(Qt::black).rgb(), Qt::MaskOutColor);
    QBitmap bitmap = QBitmap::fromImage(img_copy, Qt::DiffuseAlphaDither);

    return new Region(bitmap);
}

void 
LSHelper::roundBlurRegion(EffectWindow *w, Region *blur_region)
{
    if(blur_region->isEmpty()) {
        return;
    }

    if(!m_managed.contains(w)) {
        return;
    }

    const QRectF geo(w->frameGeometry());

    QRectF maximized_area = effects->clientArea(MaximizeArea, w);
    if (maximized_area == geo && m_disabledForMaximized) {
        return;
    }

	Region top_left = *m_maskRegions[TopLeft];
    top_left.translate(0-m_shadowOffset+1, 0-m_shadowOffset+1);  
    *blur_region = blur_region->subtracted(top_left);  

    Region top_right = *m_maskRegions[TopRight];
    top_right.translate(geo.width() - m_size-1, 0-m_shadowOffset+1);   
    *blur_region = blur_region->subtracted(top_right);  

    Region bottom_right = *m_maskRegions[BottomRight];
    bottom_right.translate(geo.width() - m_size-1, geo.height()-m_size-1);    
    *blur_region = blur_region->subtracted(bottom_right);     

    Region bottom_left = *m_maskRegions[BottomLeft];
    bottom_left.translate(0-m_shadowOffset+1, geo.height()-m_size-1);
    *blur_region = blur_region->subtracted(bottom_left);
}

void
LSHelper::roundBlurRegion(EffectWindow *w, RegionF *blur_region)
{
    if (blur_region->isEmpty()) {
        return;
    }

    if (!m_managed.contains(w)) {
        return;
    }

    const RectF geo(w->frameGeometry());

    RectF maximized_area = effects->clientArea(MaximizeArea, w);
    if (maximized_area == geo && m_disabledForMaximized) {
        return;
    }

    Region top_left = *m_maskRegions[TopLeft];
    top_left.translate(0 - m_shadowOffset + 1, 0 - m_shadowOffset + 1);
    *blur_region = *blur_region - RegionF(top_left);

    Region top_right = *m_maskRegions[TopRight];
    top_right.translate(geo.width() - m_size - 1, 0 - m_shadowOffset + 1);
    *blur_region = *blur_region - RegionF(top_right);

    Region bottom_right = *m_maskRegions[BottomRight];
    bottom_right.translate(geo.width() - m_size - 1, geo.height() - m_size - 1);
    *blur_region = *blur_region - RegionF(bottom_right);

    Region bottom_left = *m_maskRegions[BottomLeft];
    bottom_left.translate(0 - m_shadowOffset + 1, geo.height() - m_size - 1);
    *blur_region = *blur_region - RegionF(bottom_left);
}

QPainterPath
LSHelper::superellipse(float size, int n, int translate)
{
    float n2 = 2.0 / n;

    int steps = 360;

    float step = (2 * M_PI) / steps;

    QPainterPath path;
    path.moveTo(2*size, size);

    for (int i = 1; i < steps; ++i)
    {
        float t = i * step;

        float cosT = qCos(t);
        float sinT = qSin(t);

        float x = size + (qPow(qAbs(cosT), n2) * size * signum(cosT));
        float y = size - (qPow(qAbs(sinT), n2) * size * signum(sinT));

        path.lineTo(x, y);

        //qCWarning(LSHELPER) << "x: " << x << ", y: " << y << ", t: " << t << ", size: " << size << ", sinT: " << sinT << ", cosT: " << cosT << ", n2: " << n2;
    }
    path.lineTo(2*size, size);

    path.translate(translate,translate);

    return path;
}

QImage
LSHelper::genMaskImg(int size, bool mask, bool outer_rect)
{
    QImage img(size*2, size*2, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    QRect r(img.rect());
    int offset_decremented;
    if(outer_rect) {
        offset_decremented = m_shadowOffset-1;
    } else {
        offset_decremented = m_shadowOffset;
    }

    if(mask) {
        p.fillRect(img.rect(), Qt::black);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.setRenderHint(QPainter::Antialiasing);
        if (m_cornersType == SquircledCorners) {
            const QPainterPath squircle1 = superellipse((size-m_shadowOffset), m_squircleRatio, m_shadowOffset);
            p.drawPolygon(squircle1.toFillPolygon());
        } else {
            p.drawEllipse(r.adjusted(m_shadowOffset,m_shadowOffset,-m_shadowOffset,-m_shadowOffset));
        }
    } else {
        p.setPen(Qt::NoPen);
        p.setRenderHint(QPainter::Antialiasing);
        r.adjust(offset_decremented, offset_decremented, -offset_decremented, -offset_decremented);
        if(outer_rect) {
            p.setBrush(QColor(0, 0, 0, 255));
        } else {
            p.setBrush(QColor(255, 255, 255, 255));
        }
        if (m_cornersType == SquircledCorners) {
            const QPainterPath squircle2 = superellipse((size-offset_decremented), m_squircleRatio, offset_decremented);
            p.drawPolygon(squircle2.toFillPolygon());            
        } else {
            p.drawEllipse(r);
        }
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setBrush(Qt::black);
        r.adjust(1, 1, -1, -1);
        if (m_cornersType == SquircledCorners) {
            const QPainterPath squircle3 = superellipse((size-(offset_decremented+1)), m_squircleRatio, (offset_decremented+1));
            p.drawPolygon(squircle3.toFillPolygon());
        } else {
            p.drawEllipse(r);
        }
    }
    p.end();

    return img;
}

bool 
LSHelper::hasShadow(EffectWindow *w)
{
    if(w->expandedGeometry().size() != w->frameGeometry().size())
        return true;
    return false;
}

bool 
LSHelper::isManagedWindow(EffectWindow *w)
{
    if (w->isDesktop()
            || w->isFullScreen()
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

    //qCWarning(LSHELPER) << w->windowRole() << w->windowType() << w->windowClass();
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

    if(w->windowClass().contains("jetbrains", Qt::CaseInsensitive) && w->caption().contains(QRegularExpression ("win[0-9]+")))
        return false;

    if (w->windowClass().contains("plasma", Qt::CaseInsensitive) && !w->isNormalWindow() && !w->isDialog() && !w->isModal())
        return false;

    //qCWarning(LSHELPER) << w->windowClass() << w->windowClass().contains("xwaylandvideobridge", Qt::CaseInsensitive);
    return true;
}

void
LSHelper::blurWindowAdded(EffectWindow *w)
{
    if(isManagedWindow(w)) {
        m_managed.append(w);
    }
}

void
LSHelper::blurWindowDeleted(EffectWindow *w)
{
    if(m_managed.contains(w)) {
        m_managed.removeAll(w);
    }
}

} //namespace
