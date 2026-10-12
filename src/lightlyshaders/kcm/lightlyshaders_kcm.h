#pragma once

#include <KCModule>

#include "ui_lightlyshaders_config.h"

namespace KWin
{

class LightlyShadersKCM : public KCModule
{
    Q_OBJECT
public:
    explicit LightlyShadersKCM(QObject *parent, const KPluginMetaData &data);

    void save() override;

private:
    // Greys out the settings that have no effect with the current choices
    void updateEnabledWidgets();

    Ui::LightlyShadersKCM ui;
};

} // namespace KWin
