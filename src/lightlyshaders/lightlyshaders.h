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

#include <kwinoffscreeneffect.h>

#include "lshelper.h"

namespace KWin {

class GLTexture;

class Q_DECL_EXPORT LightlyShadersEffect : public OffscreenEffect
{
    Q_OBJECT
public:
    LightlyShadersEffect();
    ~LightlyShadersEffect();

    static bool supported();
    static bool enabledByDefault();

    void setRoundness(const int r, EffectScreen *s);
    void reconfigure(ReconfigureFlags flags) override;
    void paintScreen(int mask, const QRegion &region, ScreenPaintData &data) override;
    void prePaintWindow(EffectWindow* w, WindowPrePaintData& data, std::chrono::milliseconds time) override;
    void drawWindow(EffectWindow* w, int mask, const QRegion& region, WindowPaintData& data) override;
    virtual int requestedEffectChainPosition() const override { return 99; }

protected Q_SLOTS:
    void windowAdded(EffectWindow *window);
    void windowDeleted(EffectWindow *window);
    void windowMaximizedStateChanged(EffectWindow *window, bool horizontal, bool vertical);
    void windowFullScreenChanged(EffectWindow *window);

private:
    enum { Top = 0, Bottom, NShad };

    struct LSWindowStruct
    {
        bool skipEffect;
        bool isManaged;
    };

    struct LSScreenStruct
    {
        bool configured=false;
        qreal scale=1.0;
        float sizeScaled;
    };

    bool isValidWindow(EffectWindow *w);

    void fillRegion(const QRegion &reg, const QColor &c);
    QRectF scale(const QRectF rect, qreal scaleFactor);

    LSHelper *m_helper;

    int m_size, m_innerOutlineWidth, m_outerOutlineWidth, m_roundness, m_shadowOffset, m_squircleRatio, m_cornersType;
    bool m_innerOutline, m_outerOutline, m_darkTheme, m_disabledForMaximized;
    QColor m_innerOutlineColor, m_outerOutlineColor;
    std::unique_ptr<GLShader> m_shader;
    QSize m_corner;

    std::unordered_map<EffectScreen *, LSScreenStruct> m_screens;
    QMap<EffectWindow *, LSWindowStruct> m_windows;
};

} // namespace KWin

#endif //LIGHTLYSHADERS_H
