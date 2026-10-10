#pragma once

#include <effect/effectwindow.h>

#include <QSet>

namespace KWin
{

// Corner shape settings and window bookkeeping shared by the
// LightlyShaders effect and the LightlyShaders blur effect.
class LSHelper
{
public:
    enum CornersType {
        RoundedCorners = 0,
        SquircledCorners
    };

    void reconfigure();

    void windowAdded(EffectWindow *w);
    void windowDeleted(EffectWindow *w);

    // Whether the window gets rounded corners at all, decided once when it's added
    bool isManaged(EffectWindow *w) const;
    // Whether the corners should be drawn right now (not fullscreen, not skipped while maximized or tiled)
    bool shouldRound(EffectWindow *w) const;
    // Whether the window is big enough to have rounded corners. Tiny helper windows
    // would otherwise be drawn as a small circle or outline.
    bool cornersFit(EffectWindow *w) const;

    int roundness() const
    {
        return m_size;
    }
    int shadowOffset() const
    {
        return m_shadowOffset;
    }
    CornersType cornersType() const
    {
        return m_cornersType;
    }
    int squircleRatio() const
    {
        return m_squircleRatio;
    }

private:
    static bool isManagedWindow(EffectWindow *w);
    static bool hasShadow(EffectWindow *w);

    int m_size = 0;
    CornersType m_cornersType = RoundedCorners;
    int m_squircleRatio = 0;
    int m_shadowOffset = 0;
    bool m_disabledForMaximized = false;
    bool m_disabledForTiled = false;

    QSet<EffectWindow *> m_managed;
};

} // namespace KWin
