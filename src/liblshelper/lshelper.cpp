#include "lshelper.h"
#include "lightlyshaders_config.h"

#include <effect/effecthandler.h>
#include <window.h>

#include <QRegularExpression>

#include <algorithm>

namespace KWin
{

static bool isMaximized(EffectWindow *w)
{
    return w->window()->maximizeMode() == MaximizeFull;
}

static bool isTiled(EffectWindow *w)
{
    return w->window()->tile() != nullptr;
}

static bool hasShadow(EffectWindow *w)
{
    return w->expandedGeometry().size() != w->frameGeometry().size();
}

void LSHelper::reconfigure()
{
    LightlyShadersConfig::self()->load();

    m_cornersType = static_cast<CornersType>(LightlyShadersConfig::cornersType());
    m_squircleRatio = LightlyShadersConfig::squircleRatio();
    m_shadowOffset = LightlyShadersConfig::shadowOffset();
    m_size = LightlyShadersConfig::roundness();
    m_disabledForMaximized = LightlyShadersConfig::disabledForMaximized();
    m_disabledForTiled = LightlyShadersConfig::disabledForTiled();

    if (m_cornersType == SquircledCorners) {
        m_size = static_cast<int>(m_size * 0.5 * m_squircleRatio);
    }

    // The shadow sampling offset has to stay inside the corner
    m_shadowOffset = std::clamp(m_shadowOffset, 0, std::max(m_size - 1, 0));

    m_excludedClasses.clear();
    const QStringList excluded = LightlyShadersConfig::excludedWindowClasses().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &windowClass : excluded) {
        if (const QString trimmed = windowClass.trimmed(); !trimmed.isEmpty()) {
            m_excludedClasses.append(trimmed);
        }
    }

    // The excluded classes may have changed
    m_managed.clear();
    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        windowAdded(window);
    }
}

bool LSHelper::isManagedWindow(EffectWindow *w) const
{
    // Shells, docks and launchers that draw their own shape, matched by window class
    // when they don't have a server side decoration
    static const QStringList undecoratedExclusions = {
        QStringLiteral("plasma"),
        QStringLiteral("krunner"),
        QStringLiteral("sddm"),
        QStringLiteral("vmware-user"),
        QStringLiteral("latte-dock"),
        QStringLiteral("lattedock"),
        QStringLiteral("plank"),
        QStringLiteral("cairo-dock"),
        QStringLiteral("albert"),
        QStringLiteral("ulauncher"),
        QStringLiteral("ksplash"),
        QStringLiteral("ksmserver"),
    };

    if (w->isDesktop() || w->isPopupMenu() || w->isTooltip() || w->isSpecialWindow() || w->isDropdownMenu() || w->isPopupWindow() || w->isLockScreen()
        || w->isSplash() || w->isOnScreenDisplay() || w->isUtility() || w->isDock() || w->isToolbar() || w->isMenu()) {
        return false;
    }

    const QString windowClass = w->windowClass();
    const auto classContains = [&windowClass](const QString &name) {
        return windowClass.contains(name, Qt::CaseInsensitive);
    };

    if (std::any_of(m_excludedClasses.cbegin(), m_excludedClasses.cend(), classContains)) {
        return false;
    }

    if (!w->hasDecoration()) {
        if (std::any_of(undecoratedExclusions.cbegin(), undecoratedExclusions.cend(), classContains)) {
            return false;
        }
        if (classContains(QStringLiteral("reaper")) && !hasShadow(w)) {
            return false;
        }
    }

    if (classContains(QStringLiteral("xwaylandvideobridge"))) {
        return false;
    }

    // JetBrains IDEs use normal windows captioned "win<N>" for their popups
    static const QRegularExpression jetbrainsPopup(QStringLiteral("win[0-9]+"));
    if (classContains(QStringLiteral("jetbrains")) && w->caption().contains(jetbrainsPopup)) {
        return false;
    }

    if (classContains(QStringLiteral("plasma")) && !w->isNormalWindow() && !w->isDialog() && !w->isModal()) {
        return false;
    }

    return true;
}

void LSHelper::windowAdded(EffectWindow *w)
{
    if (isManagedWindow(w)) {
        m_managed.insert(w);
    }
}

void LSHelper::windowDeleted(EffectWindow *w)
{
    m_managed.remove(w);
}

bool LSHelper::isManaged(EffectWindow *w) const
{
    return m_managed.contains(w);
}

bool LSHelper::shouldRound(EffectWindow *w) const
{
    return isManaged(w) && !w->isFullScreen() && !(m_disabledForMaximized && isMaximized(w)) && !(m_disabledForTiled && isTiled(w));
}

bool LSHelper::cornersFit(EffectWindow *w) const
{
    const RectF frame = w->frameGeometry();
    return frame.width() >= 2 * m_size && frame.height() >= 2 * m_size;
}

} // namespace KWin
