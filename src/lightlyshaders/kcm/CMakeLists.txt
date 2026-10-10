set(kwin_lightlyshaders_config_SOURCES lightlyshaders_kcm.cpp)

kconfig_add_kcfg_files(kwin_lightlyshaders_config_SOURCES ../lightlyshaders_config.kcfgc)

ki18n_wrap_ui(kwin_lightlyshaders_config_SOURCES lightlyshaders_config.ui)

qt_add_dbus_interface(kwin_lightlyshaders_config_SOURCES ${KWIN_EFFECTS_INTERFACE} kwineffects_interface)

add_library(kwin_lightlyshaders_config MODULE ${kwin_lightlyshaders_config_SOURCES})

target_link_libraries(kwin_lightlyshaders_config
    KWin::kwin
    Qt6::Widgets
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::ConfigWidgets
    KF6::ConfigGui
    KF6::GlobalAccel
    KF6::WindowSystem
    KF6::KCMUtils   
    KF6::I18n         

    lshelper
)

install(TARGETS kwin_lightlyshaders_config DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/configs)

