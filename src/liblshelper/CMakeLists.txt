set(lshelper_LIB_SRCS
    lshelper.h
    lshelper.cpp
)

kconfig_add_kcfg_files(lshelper_LIB_SRCS ../lightlyshaders/lightlyshaders_config.kcfgc)

add_library(lshelper ${lshelper_LIB_SRCS})

include(GenerateExportHeader)
generate_export_header(lshelper
    BASE_NAME liblshelper
    EXPORT_FILE_NAME liblshelper_export.h)

target_link_libraries(lshelper
    Qt6::Core
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
)

set_target_properties(lshelper PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

#install(TARGETS lshelper DESTINATION /usr/lib64/qt6/plugins/kwin/effects/plugins)
install(TARGETS lshelper ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
