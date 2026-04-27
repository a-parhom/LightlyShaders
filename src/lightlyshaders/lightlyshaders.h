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

#ifndef LIGHTLYSHADERS_H
#define LIGHTLYSHADERS_H

#include <effect/effecthandler.h>
#include <effect/offscreeneffect.h>

#include <memory>
#include "lshelper.h"

namespace KWin
{

    class GLTexture;

    class Q_DECL_EXPORT LightlyShadersEffect : public OffscreenEffect
    {
        Q_OBJECT
    public:
        LightlyShadersEffect();
        ~LightlyShadersEffect();

        static bool supported();
        static bool enabledByDefault();

        void setRoundness(const int r, LogicalOutput *s);

        void reconfigure(ReconfigureFlags flags) override;
        void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *s) override;
        void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds time) override;
        void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &region, WindowPaintData &data) override;
        virtual int requestedEffectChainPosition() const override { return 99; }

    protected Q_SLOTS:
        void windowAdded(EffectWindow *window);
        void windowDeleted(EffectWindow *window);
        void windowMaximizedStateChanged(EffectWindow *window, bool horizontal, bool vertical);
        void windowFullScreenChanged(EffectWindow *window);

    private:
        enum
        {
            Top = 0,
            Bottom,
            NShad
        };

        struct LSWindowStruct
        {
            bool skipEffect;
            bool isManaged;
        };

        struct LSScreenStruct
        {
            bool configured = false;
            qreal scale = 1.0;
            float sizeScaled = 0.0f;
        };

        bool isValidWindow(EffectWindow *w);

        void fillRegion(const QRegion &reg, const QColor &c);
        QRectF scale(const QRectF rect, qreal scaleFactor);

        std::unique_ptr<LSHelper> m_helper;

        int m_size = 0;
        int m_innerOutlineWidth = 0;
        int m_outerOutlineWidth = 0;
        int m_roundness = 0;
        int m_shadowOffset = 0;
        int m_squircleRatio = 0;
        int m_cornersType = 0;
        bool m_innerOutline = false;
        bool m_outerOutline = false;
        bool m_darkTheme = false;
        bool m_disabledForMaximized = false;
        float m_defaultScale = 1.0f; // Para modo X11 onde LogicalOutput é nullptr
        QColor m_innerOutlineColor, m_outerOutlineColor;
        std::unique_ptr<GLShader> m_shader;
        QSize m_corner;

        std::unordered_map<LogicalOutput *, LSScreenStruct> m_screens;
        QMap<EffectWindow *, LSWindowStruct> m_windows;
    };

} // namespace KWin

#endif // LIGHTLYSHADERS_H
