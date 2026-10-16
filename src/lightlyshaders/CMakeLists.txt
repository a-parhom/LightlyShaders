add_subdirectory(kcm)

set(LIGHTLYSHADERS kwin_effect_lightlyshaders)

set(LIGHTLYSHADERS_SRCS
    lightlyshaders.h
    lightlyshaders.qrc
    lightlyshaders.cpp
)

kconfig_add_kcfg_files(LIGHTLYSHADERS_SRCS lightlyshaders_config.kcfgc)

add_library(${LIGHTLYSHADERS} MODULE ${LIGHTLYSHADERS_SRCS})
set_property(TARGET ${LIGHTLYSHADERS} APPEND PROPERTY
    AUTOMOC_MACRO_NAMES KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED
)

target_link_libraries(${LIGHTLYSHADERS}

    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::DBus

    KWin::kwin
    epoxy::epoxy
    GL

    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::ConfigWidgets
    KF6::GuiAddons
    KF6::WindowSystem

    lshelper
)


install(TARGETS ${LIGHTLYSHADERS} DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins)
