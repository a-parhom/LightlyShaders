find_package(KWinX11 REQUIRED)

set(LIGHTLYSHADERS_X11 kwin_effect_lightlyshaders_x11)

set(LIGHTLYSHADERS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../lightlyshaders.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lightlyshaders.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/../lightlyshaders.cpp
)

kconfig_add_kcfg_files(LIGHTLYSHADERS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../lightlyshaders_config.kcfgc)

add_library(${LIGHTLYSHADERS_X11} MODULE ${LIGHTLYSHADERS_SRCS})

target_include_directories(${LIGHTLYSHADERS_X11} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(${LIGHTLYSHADERS_X11}
    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::DBus
    KWinX11::kwin
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

install(TARGETS ${LIGHTLYSHADERS_X11} DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin-x11/effects/plugins)
