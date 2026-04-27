#include "lshelper.h"
#include "lightlyshaders_config.h"

#include <QBitmap>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QRegularExpression>
#include <numbers>

Q_LOGGING_CATEGORY(LSHELPER, "liblshelper", QtWarningMsg)

namespace KWin
{

    LSHelper::LSHelper() : QObject()
    {
        // Smart pointers são inicializados automaticamente
    }

    LSHelper::~LSHelper()
    {
        // Smart pointers gerenciam a memória automaticamente
        // Não há necessidade de delete explícito
        // Arrays m_maskRegionsSmart são destruídos automaticamente

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

        if (m_cornersType == SquircledCorners)
        {
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

        m_maskRegionsSmart[TopLeft] = std::make_unique<QRegion>(createMaskRegion(img, size, TopLeft));
        m_maskRegionsSmart[TopRight] = std::make_unique<QRegion>(createMaskRegion(img, size, TopRight));
        m_maskRegionsSmart[BottomRight] = std::make_unique<QRegion>(createMaskRegion(img, size, BottomRight));
        m_maskRegionsSmart[BottomLeft] = std::make_unique<QRegion>(createMaskRegion(img, size, BottomLeft));
    }

    QRegion
    LSHelper::createMaskRegion(QImage img, int size, int corner)
    {
        QImage img_copy;

        switch (corner)
        {
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

        return QRegion(bitmap); // Retorno por valor
    }

    void
    LSHelper::roundBlurRegion(EffectWindow *w, QRegion *blur_region)
    {
        if (blur_region->isEmpty())
        {
            return;
        }

        if (!m_managed.contains(w))
        {
            return;
        }

        const QRectF geo(w->frameGeometry());

        QRectF maximized_area = effects->clientArea(MaximizeArea, w);
        if (maximized_area == geo && m_disabledForMaximized)
        {
            return;
        }

        QRegion top_left = *m_maskRegionsSmart[TopLeft];
        top_left.translate(0 - m_shadowOffset + 1, 0 - m_shadowOffset + 1);
        *blur_region = blur_region->subtracted(top_left);

        QRegion top_right = *m_maskRegionsSmart[TopRight];
        top_right.translate(geo.width() - m_size - 1, 0 - m_shadowOffset + 1);
        *blur_region = blur_region->subtracted(top_right);

        QRegion bottom_right = *m_maskRegionsSmart[BottomRight];
        bottom_right.translate(geo.width() - m_size - 1, geo.height() - m_size - 1);
        *blur_region = blur_region->subtracted(bottom_right);

        QRegion bottom_left = *m_maskRegionsSmart[BottomLeft];
        bottom_left.translate(0 - m_shadowOffset + 1, geo.height() - m_size - 1);
        *blur_region = blur_region->subtracted(bottom_left);
    }

    QPainterPath
    LSHelper::superellipse(float size, int n, int translate)
    {
        float n2 = 2.0 / n;

        static constexpr int SUPERELLIPSE_STEPS = 360;
        static constexpr float SUPERELLIPSE_STEP = (2.0f * std::numbers::pi_v<float>) / SUPERELLIPSE_STEPS;

        const int steps = SUPERELLIPSE_STEPS;
        const float step = SUPERELLIPSE_STEP;

        QPainterPath path;
        path.moveTo(2 * size, size);

        for (int i = 1; i < steps; ++i)
        {
            float t = i * step;

            float cosT = qCos(t);
            float sinT = qSin(t);

            float x = size + (qPow(qAbs(cosT), n2) * size * signum(cosT));
            float y = size - (qPow(qAbs(sinT), n2) * size * signum(sinT));

            path.lineTo(x, y);
        }
        path.lineTo(2 * size, size);

        path.translate(translate, translate);

        return path;
    }

    QImage
    LSHelper::genMaskImg(int size, bool mask, bool outer_rect)
    {
        QImage img(size * 2, size * 2, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        QRect r(img.rect());
        const int offset_decremented = outer_rect ? m_shadowOffset - 1 : m_shadowOffset;

        if (mask)
        {
            p.fillRect(img.rect(), Qt::black);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::black);
            p.setRenderHint(QPainter::Antialiasing);
            if (m_cornersType == SquircledCorners)
            {
                const QPainterPath squircle1 = superellipse((size - m_shadowOffset), m_squircleRatio, m_shadowOffset);
                p.drawPolygon(squircle1.toFillPolygon());
            }
            else
            {
                p.drawEllipse(r.adjusted(m_shadowOffset, m_shadowOffset, -m_shadowOffset, -m_shadowOffset));
            }
        }
        else
        {
            p.setPen(Qt::NoPen);
            p.setRenderHint(QPainter::Antialiasing);
            r.adjust(offset_decremented, offset_decremented, -offset_decremented, -offset_decremented);
            if (outer_rect)
            {
                p.setBrush(QColor(0, 0, 0, 255));
            }
            else
            {
                p.setBrush(QColor(255, 255, 255, 255));
            }
            if (m_cornersType == SquircledCorners)
            {
                const QPainterPath squircle2 = superellipse((size - offset_decremented), m_squircleRatio, offset_decremented);
                p.drawPolygon(squircle2.toFillPolygon());
            }
            else
            {
                p.drawEllipse(r);
            }
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            p.setBrush(Qt::black);
            r.adjust(1, 1, -1, -1);
            if (m_cornersType == SquircledCorners)
            {
                const QPainterPath squircle3 = superellipse((size - (offset_decremented + 1)), m_squircleRatio, (offset_decremented + 1));
                p.drawPolygon(squircle3.toFillPolygon());
            }
            else
            {
                p.drawEllipse(r);
            }
        }
        p.end();

        return img;
    }

    bool
    LSHelper::hasShadow(EffectWindow *w) const
    {
        if (w->expandedGeometry().size() != w->frameGeometry().size())
            return true;
        return false;
    }

    namespace
    {
        // Cache estático da blacklist de classes de janela
        const QRegularExpression &getWindowClassBlacklistRegex()
        {
            static const QRegularExpression re(
                QStringLiteral("(plasma|krunner|sddm|vmware-user|latte-dock|lattedock|"
                               "plank|cairo-dock|albert|ulauncher|ksplash|ksmserver|"
                               "xwaylandvideobridge|reaper)"),
                QRegularExpression::CaseInsensitiveOption);
            return re;
        }
    }

    bool
    LSHelper::isManagedWindow(EffectWindow *w)
    {
        // Etapa 1: Verificar tipos especiais de janela
        if (isSpecialWindowType(w))
            return false;

        // Etapa 2: Verificar classes blacklisted
        if (isBlacklistedClass(w))
            return false;

        // Etapa 3: Verificar casos especiais de aplicações
        if (isSpecialApplicationCase(w))
            return false;

        return true;
    }

    bool
    LSHelper::isSpecialWindowType(EffectWindow *w) const
    {
        if (w->isDesktop())
            return true;
        if (w->isFullScreen())
            return true;
        if (w->isPopupMenu())
            return true;
        if (w->isTooltip())
            return true;
        if (w->isSpecialWindow())
            return true;
        if (w->isDropdownMenu())
            return true;
        if (w->isPopupWindow())
            return true;
        if (w->isLockScreen())
            return true;
        if (w->isSplash())
            return true;
        if (w->isOnScreenDisplay())
            return true;
        if (w->isUtility())
            return true;
        if (w->isDock())
            return true;
        if (w->isToolbar())
            return true;
        if (w->isMenu())
            return true;
        return false;
    }

    bool
    LSHelper::isBlacklistedClass(EffectWindow *w) const
    {
        const QString windowClass = w->windowClass();

        // Verificar blacklist usando regex (uma única passagem)
        if (getWindowClassBlacklistRegex().match(windowClass).hasMatch())
        {
            // Caso especial: REAPER com shadow é permitido
            if (windowClass.contains(QLatin1String("reaper"), Qt::CaseInsensitive) && hasShadow(w))
            {
                return false;
            }
            // xwaylandvideobridge sempre é rejeitado
            if (windowClass.contains(QLatin1String("xwaylandvideobridge"), Qt::CaseInsensitive))
            {
                return true;
            }
            // Janelas decoradas são permitidas
            if (w->hasDecoration())
            {
                return false;
            }
            return true;
        }

        return false;
    }

    bool
    LSHelper::isSpecialApplicationCase(EffectWindow *w) const
    {
        const QString windowClass = w->windowClass();

        // JetBrains IDE tool windows (padrão win[0-9]+ no título) devem ser rejeitadas
        if (windowClass.contains(QLatin1String("jetbrains"), Qt::CaseInsensitive))
        {
            static const QRegularExpression toolWindowRe(QStringLiteral("win[0-9]+"));
            if (w->caption().contains(toolWindowRe))
                return true; // Rejeitar tool windows do JetBrains
        }

        // Plasma windows que não são janelas normais/diálogos/modais devem ser rejeitadas
        if (windowClass.contains(QLatin1String("plasma"), Qt::CaseInsensitive))
        {
            if (!w->isNormalWindow() && !w->isDialog() && !w->isModal())
            {
                return true; // Rejeitar janelas Plasma especiais
            }
        }

        // Permitir todas as outras janelas, incluindo janelas sem decoração nativa (frameless)
        return false;
    }

    void
    LSHelper::blurWindowAdded(EffectWindow *w)
    {
        if (isManagedWindow(w))
        {
            m_managed.append(w);
        }
    }

    void
    LSHelper::blurWindowDeleted(EffectWindow *w)
    {
        if (m_managed.contains(w))
        {
            m_managed.removeAll(w);
        }
    }

} // namespace
