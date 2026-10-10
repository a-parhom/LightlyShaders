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
    void updateSquircleRatioEnabled();
    void updateInactiveOutlinesEnabled();

    Ui::LightlyShadersKCM ui;
};

} // namespace KWin
