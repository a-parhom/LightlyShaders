/*
    SPDX-FileCopyrightText: 2010 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "blur_config.h"

// KConfigSkeleton
#include "blurconfig.h"

#include <KPluginFactory>
#include <kwineffects_interface.h>

namespace KWin
{

// Inside the namespace, the macro can't take a qualified class name outside of KWin's own build
K_PLUGIN_CLASS(BlurEffectConfig)

BlurEffectConfig::BlurEffectConfig(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    ui.setupUi(widget());
    BlurConfig::instance(QStringLiteral("kwinrc"));
    addConfig(BlurConfig::self(), widget());
}

void BlurEffectConfig::save()
{
    KCModule::save();

    OrgKdeKwinEffectsInterface interface(QStringLiteral("org.kde.KWin"),
                                         QStringLiteral("/Effects"),
                                         QDBusConnection::sessionBus());
    interface.reconfigureEffect(QStringLiteral("lightlyshaders_blur"));
}

} // namespace KWin

#include "blur_config.moc"

#include "moc_blur_config.cpp"
