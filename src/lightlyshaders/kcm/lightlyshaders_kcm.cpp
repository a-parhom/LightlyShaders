#include "lightlyshaders_kcm.h"
#include "kwineffects_interface.h"
#include "lightlyshaders_config.h"

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

    connect(ui.kcfg_InactiveOutlines, &QCheckBox::toggled, this, &LightlyShadersKCM::updateInactiveOutlinesEnabled);
    updateInactiveOutlinesEnabled();
}

void LightlyShadersKCM::save()
{
    KCModule::save();

    OrgKdeKwinEffectsInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Effects"), QDBusConnection::sessionBus());
    interface.reconfigureEffect(QStringLiteral("kwin_effect_lightlyshaders"));
    interface.reconfigureEffect(QStringLiteral("lightlyshaders_blur"));
}

void LightlyShadersKCM::updateSquircleRatioEnabled()
{
    const bool squircle = ui.kcfg_CornersType->currentIndex() == LightlyShadersConfig::EnumCornersType::SquircledCorners;
    ui.squircleRatioLabel->setEnabled(squircle);
    ui.kcfg_SquircleRatio->setEnabled(squircle);
}

void LightlyShadersKCM::updateInactiveOutlinesEnabled()
{
    const bool enabled = ui.kcfg_InactiveOutlines->isChecked();
    ui.inactiveInnerOutlineLabel->setEnabled(enabled);
    ui.kcfg_InactiveInnerOutlineColor->setEnabled(enabled);
    ui.inactiveOuterOutlineLabel->setEnabled(enabled);
    ui.kcfg_InactiveOuterOutlineColor->setEnabled(enabled);
}

} // namespace KWin

#include "lightlyshaders_kcm.moc"
