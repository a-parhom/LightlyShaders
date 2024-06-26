/*
 *   Copyright © 2015 Robert Metsäranta <therealestrob@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; see the file COPYING.  if not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#include "lightlyshaders.h"
#include "lightlyshaders_config.h"
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QWindow>
#include <opengl/glutils.h>
#include <effect/effect.h>
#include <core/renderviewport.h>
#include <QMatrix4x4>
#include <KConfigGroup>
#include <QRegularExpression>
#include <QBitmap>
#include <KWindowEffects>

Q_LOGGING_CATEGORY(LIGHTLYSHADERS, "kwin_effect_lightlyshaders", QtWarningMsg)

static void ensureResources()
{
    // Must initialize resources manually because the effect is a static lib.
    Q_INIT_RESOURCE(lightlyshaders);
}

namespace KWin {

KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED(  LightlyShadersEffect,
                                        "lightlyshaders.json",
                                        return LightlyShadersEffect::supported();,
                                        return LightlyShadersEffect::enabledByDefault();)

LightlyShadersEffect::LightlyShadersEffect() : OffscreenEffect()
{
    ensureResources();

    m_helper = new LSHelper();
    reconfigure(ReconfigureAll);

    m_shader = std::unique_ptr<GLShader>(ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture, QStringLiteral(""), QStringLiteral(":/effects/lightlyshaders/shaders/lightlyshaders.frag")));

    if (!m_shader) {
        qCWarning(LIGHTLYSHADERS) << "Failed to load shader";
        return;
    }

    if (m_shader->isValid())
    {
        const auto stackingOrder = effects->stackingOrder();
        for (EffectWindow *window : stackingOrder) {
            windowAdded(window);
        }

        connect(effects, &EffectsHandler::windowAdded, this, &LightlyShadersEffect::windowAdded);
        connect(effects, &EffectsHandler::windowDeleted, this, &LightlyShadersEffect::windowDeleted);

        qCWarning(LIGHTLYSHADERS) << "LightlyShaders loaded.";
    }
    else
        qCWarning(LIGHTLYSHADERS) << "LightlyShaders: no valid shaders found! LightlyShaders will not work.";
}

LightlyShadersEffect::~LightlyShadersEffect()
{
    m_windows.clear();
}

void
LightlyShadersEffect::windowDeleted(EffectWindow *w)
{
    m_windows.remove(w);
}

void
LightlyShadersEffect::windowAdded(EffectWindow *w)
{
    m_windows[w].isManaged = false;

    if(!m_helper->isManagedWindow(w))
        return;

    m_windows[w].isManaged = true;
    m_windows[w].skipEffect = false;

    connect(w, &EffectWindow::windowMaximizedStateChanged,
            this, &LightlyShadersEffect::windowMaximizedStateChanged);
    connect(w, &EffectWindow::windowFullScreenChanged,
            this, &LightlyShadersEffect::windowFullScreenChanged);

    QRectF maximized_area = effects->clientArea(MaximizeArea, w);
    if (maximized_area == w->frameGeometry() && m_disabledForMaximized)
        m_windows[w].skipEffect = true;

    redirect(w);
    setShader(w, m_shader.get());
}

void
LightlyShadersEffect::windowFullScreenChanged(EffectWindow *w)
{
    if(w->isFullScreen()) {
        m_windows[w].isManaged = false;
    } else {
        m_windows[w].isManaged = true;
    }
}

void
LightlyShadersEffect::windowMaximizedStateChanged(EffectWindow *w, bool horizontal, bool vertical)
{
    if (!m_disabledForMaximized) return;

    if ((horizontal == true) && (vertical == true)) {
        m_windows[w].skipEffect = true;
    } else {
        m_windows[w].skipEffect = false;
    }
}

void
LightlyShadersEffect::setRoundness(const int r, Output *s)
{
    m_size = r;
    m_screens[s].sizeScaled = float(r)*m_screens[s].scale;
    m_corner = QSize(m_size+(m_shadowOffset-1), m_size+(m_shadowOffset-1));
}

void
LightlyShadersEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)

    LightlyShadersConfig::self()->load();

    m_innerOutlineWidth = LightlyShadersConfig::innerOutlineWidth();
    m_outerOutlineWidth = LightlyShadersConfig::outerOutlineWidth();
    m_innerOutline = LightlyShadersConfig::innerOutline();
    m_outerOutline = LightlyShadersConfig::outerOutline();
    m_innerOutlineColor = LightlyShadersConfig::innerOutlineColor();
    m_outerOutlineColor = LightlyShadersConfig::outerOutlineColor();
    m_disabledForMaximized = LightlyShadersConfig::disabledForMaximized();
    m_shadowOffset = LightlyShadersConfig::shadowOffset();
    m_squircleRatio = LightlyShadersConfig::squircleRatio();
    m_cornersType = LightlyShadersConfig::cornersType();

    m_helper->reconfigure();
    m_roundness = m_helper->roundness();

    if(m_shadowOffset>=m_roundness) {
        m_shadowOffset = m_roundness-1;
    }

    if(!m_innerOutline) {
        m_innerOutlineWidth = 0.0;
    }
    if(!m_outerOutline) {
        m_outerOutlineWidth = 0.0;
    }

    const auto screens = effects->screens();
    for(Output *s : screens)
    {
        if (effects->waylandDisplay() == nullptr) {
            s = nullptr;
        }
        setRoundness(m_roundness, s);

        if (effects->waylandDisplay() == nullptr) {
            break;
        }
    }

    effects->addRepaintFull();
}

void
LightlyShadersEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *s)
{
    bool set_roundness = false;

    if (!m_screens[s].configured) {
        m_screens[s].configured = true;
        set_roundness = true;
    }

    qreal scale = viewport.scale();

    if(scale != m_screens[s].scale) {
        m_screens[s].scale = scale;
        set_roundness = true;
    }

    if(set_roundness) {
        setRoundness(m_roundness, s);
        m_helper->reconfigure();
    }

    effects->paintScreen(renderTarget, viewport, mask, region, s);
}

void
LightlyShadersEffect::prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds time)
{
    if (!isValidWindow(w) )
    {
        effects->prePaintWindow(w, data, time);
        return;
    }

    Output *s = w->screen();
    if (effects->waylandDisplay() == nullptr) {
        s = nullptr;
    }

    const QRectF geo(w->frameGeometry());
    for (int corner = 0; corner < LSHelper::NTex; ++corner)
    {
        QRegion reg = QRegion(scale(m_helper->m_maskRegions[corner]->boundingRect(), m_screens[s].scale).toRect());
        switch(corner) {
            case LSHelper::TopLeft:
                reg.translate(geo.x()-m_shadowOffset, geo.y()-m_shadowOffset);
                break;
            case LSHelper::TopRight:
                reg.translate(geo.x() + geo.width() - m_size, geo.y()-m_shadowOffset);
                break;
            case LSHelper::BottomRight:
                reg.translate(geo.x() + geo.width() - m_size-1, geo.y()+geo.height()-m_size-1);
                break;
            case LSHelper::BottomLeft:
                reg.translate(geo.x()-m_shadowOffset+1, geo.y()+geo.height()-m_size-1);
                break;
            default:
                break;
        }

        data.opaque -= reg;
    }

    effects->prePaintWindow(w, data, time);
}

bool
LightlyShadersEffect::isValidWindow(EffectWindow *w)
{
    if (!m_shader->isValid()
            || !m_windows[w].isManaged
            || m_windows[w].skipEffect
        )
    {
        return false;
    }
    return true;
}

void
LightlyShadersEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const QRegion &region, WindowPaintData &data)
{    
    QRectF screen = viewport.renderRect().toRect();

    if (!isValidWindow(w) || (!screen.intersects(w->frameGeometry()) && !(mask & PAINT_WINDOW_TRANSFORMED)) )
    {
        effects->drawWindow(renderTarget, viewport, w, mask, region, data);
        return;
    }

    Output *s = w->screen();
    if (effects->waylandDisplay() == nullptr) {
        s = nullptr;
    }

    QRectF geo(w->frameGeometry());
    QRectF exp_geo(w->expandedGeometry());
    QRectF contents_geo(w->contentsRect());

    const QRectF geo_scaled = scale(geo, m_screens[s].scale);
    const QRectF exp_geo_scaled = scale(exp_geo, m_screens[s].scale);

    //Draw rounded corners with shadows
    const int frameSizeLocation = m_shader->uniformLocation("frame_size");
    const int expandedSizeLocation = m_shader->uniformLocation("expanded_size");
    const int shadowSizeLocation = m_shader->uniformLocation("shadow_size");
    const int radiusLocation = m_shader->uniformLocation("radius");
    const int shadowOffsetLocation = m_shader->uniformLocation("shadow_sample_offset");
    const int innerOutlineColorLocation = m_shader->uniformLocation("inner_outline_color");
    const int outerOutlineColorLocation = m_shader->uniformLocation("outer_outline_color");
    const int innerOutlineWidthLocation = m_shader->uniformLocation("inner_outline_width");
    const int outerOutlineWidthLocation = m_shader->uniformLocation("outer_outline_width");
    const int drawInnerOutlineLocation = m_shader->uniformLocation("draw_inner_outline");
    const int drawOuterOutlineLocation = m_shader->uniformLocation("draw_outer_outline");
    const int squircleRatioLocation = m_shader->uniformLocation("squircle_ratio");
    const int isSquircleLocation = m_shader->uniformLocation("is_squircle");
    ShaderManager *sm = ShaderManager::instance();
    sm->pushShader(m_shader.get());

    //qCWarning(LIGHTLYSHADERS) << geo_scaled.width() << geo_scaled.height();
    m_shader->setUniform(frameSizeLocation, QVector2D(geo_scaled.width(), geo_scaled.height()));
    m_shader->setUniform(expandedSizeLocation, QVector2D(exp_geo_scaled.width(), exp_geo_scaled.height()));
    m_shader->setUniform(shadowSizeLocation, QVector3D(geo_scaled.x() - exp_geo_scaled.x(), geo_scaled.y()-exp_geo_scaled.y(), exp_geo_scaled.height() - geo_scaled.height() - geo_scaled.y() + exp_geo_scaled.y() ));
    m_shader->setUniform(radiusLocation, m_screens[s].sizeScaled);
    m_shader->setUniform(shadowOffsetLocation, float(m_shadowOffset*m_screens[s].scale));
    m_shader->setUniform(innerOutlineColorLocation, QVector4D(m_innerOutlineColor.red()/255.0,m_innerOutlineColor.green()/255.0,m_innerOutlineColor.blue()/255.0,m_innerOutlineColor.alpha()/255.0));
    m_shader->setUniform(outerOutlineColorLocation, QVector4D(m_outerOutlineColor.red()/255.0,m_outerOutlineColor.green()/255.0,m_outerOutlineColor.blue()/255.0,m_outerOutlineColor.alpha()/255.0));
    m_shader->setUniform(innerOutlineWidthLocation, float(m_innerOutlineWidth*m_screens[s].scale));
    m_shader->setUniform(outerOutlineWidthLocation, float(m_outerOutlineWidth*m_screens[s].scale));
    m_shader->setUniform(drawInnerOutlineLocation, m_innerOutline);
    m_shader->setUniform(drawOuterOutlineLocation, m_outerOutline);
    m_shader->setUniform(squircleRatioLocation, m_squircleRatio);
    m_shader->setUniform(isSquircleLocation, (m_cornersType == LSHelper::SquircledCorners));

    glActiveTexture(GL_TEXTURE0);

    OffscreenEffect::drawWindow(renderTarget, viewport, w, mask, region, data);

    sm->popShader();
}

QRectF
LightlyShadersEffect::scale(const QRectF rect, qreal scaleFactor)
{
    return QRectF(
        rect.x()*scaleFactor,
        rect.y()*scaleFactor,
        rect.width()*scaleFactor,
        rect.height()*scaleFactor
    );
}

bool
LightlyShadersEffect::enabledByDefault()
{
    return supported();
}

bool
LightlyShadersEffect::supported()
{
    return effects->isOpenGLCompositing();
}

} // namespace KWin

#include "lightlyshaders.moc"
