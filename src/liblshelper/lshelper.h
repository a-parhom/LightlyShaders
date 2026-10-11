#pragma once

#include <effect/effectwindow.h>

#include <QSet>
#include <QStringList>

namespace KWin
{

// Corner shape settings and window bookkeeping shared by the
// LightlyShaders effect and the LightlyShaders blur effect.
class LSHelper
{
public:
    // Reloads the settings and re-decides which windows are managed
    void reconfigure();

    // Decides whether the window is managed, again whenever a property it depends on changed
    void reevaluate(EffectWindow *w);
    void windowDeleted(EffectWindow *w);

    // Whether the window gets rounded corners at all, depending on its type and class
    bool isManaged(EffectWindow *w) const;
    // Whether the corners should be drawn in the window's current state
    // (not fullscreen, not skipped while maximized or tiled)
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
    bool isSquircle() const
    {
        return m_squircle;
    }
    int squircleRatio() const
    {
        return m_squircleRatio;
    }

private:
    bool isManagedWindow(EffectWindow *w) const;

    int m_size = 0;
    bool m_squircle = false;
    int m_squircleRatio = 0;
    int m_shadowOffset = 0;
    bool m_disabledForMaximized = false;
    bool m_disabledForTiled = false;
    QStringList m_excludedClasses;

    QSet<EffectWindow *> m_managed;
};

} // namespace KWin
