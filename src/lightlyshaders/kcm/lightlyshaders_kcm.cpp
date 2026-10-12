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

    connect(ui.kcfg_CornersType, &QComboBox::currentIndexChanged, this, &LightlyShadersKCM::updateEnabledWidgets);
    connect(ui.kcfg_InnerOutline, &QCheckBox::toggled, this, &LightlyShadersKCM::updateEnabledWidgets);
    connect(ui.kcfg_OuterOutline, &QCheckBox::toggled, this, &LightlyShadersKCM::updateEnabledWidgets);
    connect(ui.kcfg_InactiveOutlines, &QCheckBox::toggled, this, &LightlyShadersKCM::updateEnabledWidgets);
    updateEnabledWidgets();
}

void LightlyShadersKCM::save()
{
    KCModule::save();

    OrgKdeKwinEffectsInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Effects"), QDBusConnection::sessionBus());
    interface.reconfigureEffect(QStringLiteral("kwin_effect_lightlyshaders"));
    interface.reconfigureEffect(QStringLiteral("lightlyshaders_blur"));
}

void LightlyShadersKCM::updateEnabledWidgets()
{
    const bool squircle = ui.kcfg_CornersType->currentIndex() == LightlyShadersConfig::EnumCornersType::SquircledCorners;
    ui.squircleRatioLabel->setEnabled(squircle);
    ui.kcfg_SquircleRatio->setEnabled(squircle);

    const bool inner = ui.kcfg_InnerOutline->isChecked();
    ui.kcfg_InnerOutlineColor->setEnabled(inner);
    ui.innerOutlineWidthLabel->setEnabled(inner);
    ui.kcfg_InnerOutlineWidth->setEnabled(inner);

    const bool outer = ui.kcfg_OuterOutline->isChecked();
    ui.kcfg_OuterOutlineColor->setEnabled(outer);
    ui.outerOutlineWidthLabel->setEnabled(outer);
    ui.kcfg_OuterOutlineWidth->setEnabled(outer);

    const bool inactive = ui.kcfg_InactiveOutlines->isChecked();
    ui.inactiveInnerOutlineLabel->setEnabled(inactive && inner);
    ui.kcfg_InactiveInnerOutlineColor->setEnabled(inactive && inner);
    ui.inactiveOuterOutlineLabel->setEnabled(inactive && outer);
    ui.kcfg_InactiveOuterOutlineColor->setEnabled(inactive && outer);
}

} // namespace KWin

#include "lightlyshaders_kcm.moc"
