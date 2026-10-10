#pragma once

#include <core/region.h>
#include <effect/effectwindow.h>

#include <QImage>
#include <QPainterPath>
#include <QSet>

namespace KWin {

// Corner shape settings and window bookkeeping shared by the
// LightlyShaders effect and the LightlyShaders blur effect.
class LSHelper
{
public:
    enum CornersType { RoundedCorners = 0, SquircledCorners };

    void reconfigure();

    void windowAdded(EffectWindow *w);
    void windowDeleted(EffectWindow *w);

    // Whether the window gets rounded corners at all, decided once when it's added
    bool isManaged(EffectWindow *w) const;
    // Whether the corners should be drawn right now (not fullscreen, not skipped while maximized)
    bool shouldRound(EffectWindow *w) const;

    void roundBlurRegion(EffectWindow *w, RegionF *region) const;

    int roundness() const { return m_size; }
    int shadowOffset() const { return m_shadowOffset; }
    int cornersType() const { return m_cornersType; }
    int squircleRatio() const { return m_squircleRatio; }

private:
    enum { TopLeft = 0, TopRight, BottomRight, BottomLeft, NCorners };

    static bool isManagedWindow(EffectWindow *w);
    static bool hasShadow(EffectWindow *w);

    void setMaskRegions();
    QImage genMaskImg(int size) const;
    QPainterPath superellipse(float size, int n, int translate) const;

    int m_size = 0;
    int m_cornersType = RoundedCorners;
    int m_squircleRatio = 0;
    int m_shadowOffset = 0;
    bool m_disabledForMaximized = false;

    // Parts of each corner to cut from the blur region, already offset relative to
    // their corner of the window. Converted once here since blur asks every frame.
    RegionF m_cornerMasks[NCorners];
    QSet<EffectWindow *> m_managed;
};

} // namespace KWin
