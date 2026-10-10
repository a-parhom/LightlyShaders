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

#include <core/renderviewport.h>
#include <effect/effecthandler.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

Q_LOGGING_CATEGORY(LIGHTLYSHADERS, "kwin_effect_lightlyshaders", QtWarningMsg)

static void ensureResources()
{
    // Must initialize resources manually because the effect is a static lib.
    Q_INIT_RESOURCE(lightlyshaders);
}

namespace KWin {

KWIN_EFFECT_FACTORY_SUPPORTED(LightlyShadersEffect,
                              "lightlyshaders.json",
                              return LightlyShadersEffect::supported();)

static QVector4D toVector(const QColor &c)
{
    return QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

static RectF scaled(const RectF &rect, qreal scale)
{
    return RectF(rect.x() * scale, rect.y() * scale, rect.width() * scale, rect.height() * scale);
}

LightlyShadersEffect::LightlyShadersEffect()
{
    ensureResources();

    reconfigure(ReconfigureAll);

    m_shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture, QString(), QStringLiteral(":/effects/lightlyshaders/shaders/lightlyshaders.frag"));
    if (!m_shader) {
        qCWarning(LIGHTLYSHADERS) << "Failed to load shader, LightlyShaders will not work";
        return;
    }

    m_uniforms = {
        .frameSize = m_shader->uniformLocation("frame_size"),
        .expandedSize = m_shader->uniformLocation("expanded_size"),
        .shadowSize = m_shader->uniformLocation("shadow_size"),
        .radius = m_shader->uniformLocation("radius"),
        .shadowOffset = m_shader->uniformLocation("shadow_sample_offset"),
        .innerOutlineColor = m_shader->uniformLocation("inner_outline_color"),
        .outerOutlineColor = m_shader->uniformLocation("outer_outline_color"),
        .innerOutlineWidth = m_shader->uniformLocation("inner_outline_width"),
        .outerOutlineWidth = m_shader->uniformLocation("outer_outline_width"),
        .drawInnerOutline = m_shader->uniformLocation("draw_inner_outline"),
        .drawOuterOutline = m_shader->uniformLocation("draw_outer_outline"),
        .squircleRatio = m_shader->uniformLocation("squircle_ratio"),
        .isSquircle = m_shader->uniformLocation("is_squircle"),
    };

    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        windowAdded(window);
    }

    connect(effects, &EffectsHandler::windowAdded, this, &LightlyShadersEffect::windowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &LightlyShadersEffect::windowDeleted);
}

LightlyShadersEffect::~LightlyShadersEffect() = default;

void
LightlyShadersEffect::windowAdded(EffectWindow *w)
{
    m_helper.windowAdded(w);
    if (!m_helper.isManaged(w)) {
        return;
    }

    connect(w, &EffectWindow::windowFullScreenChanged, this, &LightlyShadersEffect::updateRedirect);
    connect(w, &EffectWindow::windowMaximizedStateChanged, this, [this](EffectWindow *w) {
        updateRedirect(w);
    });
    updateRedirect(w);
}

void
LightlyShadersEffect::windowDeleted(EffectWindow *w)
{
    // OffscreenEffect unredirects deleted windows by itself
    m_redirected.remove(w);
    m_helper.windowDeleted(w);
}

// Only keep a window offscreen while its corners are actually drawn, so fullscreen
// windows (and maximized ones, if disabled for them) render directly again
void
LightlyShadersEffect::updateRedirect(EffectWindow *w)
{
    const bool shouldRedirect = m_shader && m_helper.shouldRound(w);
    if (shouldRedirect == m_redirected.contains(w)) {
        return;
    }

    if (shouldRedirect) {
        redirect(w);
        setShader(w, m_shader.get());
        m_redirected.insert(w);
    } else {
        unredirect(w);
        m_redirected.remove(w);
    }
}

void
LightlyShadersEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)

    m_helper.reconfigure();

    m_innerOutline = LightlyShadersConfig::innerOutline();
    m_outerOutline = LightlyShadersConfig::outerOutline();
    m_innerOutlineWidth = m_innerOutline ? LightlyShadersConfig::innerOutlineWidth() : 0;
    m_outerOutlineWidth = m_outerOutline ? LightlyShadersConfig::outerOutlineWidth() : 0;
    m_innerOutlineColor = LightlyShadersConfig::innerOutlineColor();
    m_outerOutlineColor = LightlyShadersConfig::outerOutlineColor();

    // "Disabled for maximized" may have changed
    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        if (m_helper.isManaged(window)) {
            updateRedirect(window);
        }
    }

    effects->addRepaintFull();
}

void
LightlyShadersEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data)
{
    // The rounded corners are see-through, so the window must not occlude what is below it.
    // KWin no longer lets effects carve parts out of the opaque region, it's all or nothing.
    if (m_redirected.contains(w)) {
        data.setTranslucent();
    }

    effects->prePaintWindow(view, w, data);
}

void
LightlyShadersEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data)
{
    const bool visible = viewport.renderRect().intersects(w->expandedGeometry()) || (mask & PAINT_WINDOW_TRANSFORMED);
    if (!m_redirected.contains(w) || !visible) {
        effects->drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);
        return;
    }

    // Use the scale of the output being painted, a window can span outputs with different scales
    const qreal scale = viewport.scale();
    const RectF geo = scaled(w->frameGeometry(), scale);
    const RectF expGeo = scaled(w->expandedGeometry(), scale);

    ShaderManager *sm = ShaderManager::instance();
    sm->pushShader(m_shader.get());

    m_shader->setUniform(m_uniforms.frameSize, QVector2D(geo.width(), geo.height()));
    m_shader->setUniform(m_uniforms.expandedSize, QVector2D(expGeo.width(), expGeo.height()));
    m_shader->setUniform(m_uniforms.shadowSize, QVector3D(geo.x() - expGeo.x(), geo.y() - expGeo.y(), expGeo.height() - geo.height() - geo.y() + expGeo.y()));
    m_shader->setUniform(m_uniforms.radius, float(m_helper.roundness() * scale));
    m_shader->setUniform(m_uniforms.shadowOffset, float(m_helper.shadowOffset() * scale));
    m_shader->setUniform(m_uniforms.innerOutlineColor, toVector(m_innerOutlineColor));
    m_shader->setUniform(m_uniforms.outerOutlineColor, toVector(m_outerOutlineColor));
    m_shader->setUniform(m_uniforms.innerOutlineWidth, float(m_innerOutlineWidth * scale));
    m_shader->setUniform(m_uniforms.outerOutlineWidth, float(m_outerOutlineWidth * scale));
    m_shader->setUniform(m_uniforms.drawInnerOutline, int(m_innerOutline));
    m_shader->setUniform(m_uniforms.drawOuterOutline, int(m_outerOutline));
    m_shader->setUniform(m_uniforms.squircleRatio, m_helper.squircleRatio());
    m_shader->setUniform(m_uniforms.isSquircle, int(m_helper.cornersType() == LSHelper::SquircledCorners));

    OffscreenEffect::drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);

    sm->popShader();
}

bool
LightlyShadersEffect::supported()
{
    return effects->isOpenGLCompositing();
}

} // namespace KWin

#include "lightlyshaders.moc"
