add_subdirectory(kcm)

set(LIGHTLYSHADERS kwin_effect_lightlyshaders)

set(LIGHTLYSHADERS_SRCS
    lightlyshaders.h
    lightlyshaders.qrc
    lightlyshaders.cpp
)

kconfig_add_kcfg_files(LIGHTLYSHADERS_SRCS lightlyshaders_config.kcfgc)

add_library(${LIGHTLYSHADERS} MODULE ${LIGHTLYSHADERS_SRCS})

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

find_package(KWinX11 QUIET)
if(KWinX11_FOUND)
    add_subdirectory(x11)
endif()
