#include "lightlyshaders_kcm.h"
#include "lightlyshaders_config.h"
#include "kwineffects_interface.h"

#include <KPluginFactory>

namespace KWin
{

K_PLUGIN_CLASS(LightlyShadersKCM)

LightlyShadersKCM::LightlyShadersKCM(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    ui.setupUi(widget());
    addConfig(LightlyShadersConfig::self(), widget());

    connect(ui.kcfg_CornersType, &QComboBox::currentIndexChanged, this, &LightlyShadersKCM::updateSquircleRatioEnabled);
    updateSquircleRatioEnabled();
}

void
LightlyShadersKCM::save()
{
    KCModule::save();

    OrgKdeKwinEffectsInterface interface(QStringLiteral("org.kde.KWin"),
                                         QStringLiteral("/Effects"),
                                         QDBusConnection::sessionBus());
    interface.reconfigureEffect(QStringLiteral("kwin_effect_lightlyshaders"));
    interface.reconfigureEffect(QStringLiteral("lightlyshaders_blur"));
}

void
LightlyShadersKCM::updateSquircleRatioEnabled()
{
    const bool squircle = ui.kcfg_CornersType->currentIndex() == LightlyShadersConfig::EnumCornersType::SquircledCorners;
    ui.squircleRatioLabel->setEnabled(squircle);
    ui.kcfg_SquircleRatio->setEnabled(squircle);
}

} // namespace KWin

#include "lightlyshaders_kcm.moc"
