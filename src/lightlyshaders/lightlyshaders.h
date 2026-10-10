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

#pragma once

#include <effect/offscreeneffect.h>

#include "lshelper.h"

#include <QColor>

namespace KWin {

class GLShader;

class LightlyShadersEffect : public OffscreenEffect
{
    Q_OBJECT
public:
    LightlyShadersEffect();
    ~LightlyShadersEffect() override;

    static bool supported();

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data) override;
    void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data) override;
    int requestedEffectChainPosition() const override { return 99; }

private:
    void windowAdded(EffectWindow *w);
    void windowDeleted(EffectWindow *w);
    void updateRedirect(EffectWindow *w);

    LSHelper m_helper;
    // Windows currently drawn offscreen through the shader
    QSet<EffectWindow *> m_redirected;

    int m_innerOutlineWidth = 0;
    int m_outerOutlineWidth = 0;
    bool m_innerOutline = false;
    bool m_outerOutline = false;
    QColor m_innerOutlineColor;
    QColor m_outerOutlineColor;

    std::unique_ptr<GLShader> m_shader;
    struct {
        int frameSize;
        int expandedSize;
        int shadowSize;
        int radius;
        int shadowOffset;
        int innerOutlineColor;
        int outerOutlineColor;
        int innerOutlineWidth;
        int outerOutlineWidth;
        int drawInnerOutline;
        int drawOuterOutline;
        int squircleRatio;
        int isSquircle;
    } m_uniforms;
};

} // namespace KWin
