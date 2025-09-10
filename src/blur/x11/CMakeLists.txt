find_package(KWinX11 REQUIRED)

set(lightlyshaders_blur_x11_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../blur.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../blur.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp
)

kconfig_add_kcfg_files(lightlyshaders_blur_x11_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../blurconfig.kcfgc
)

add_library(lightlyshaders_blur_x11 MODULE ${lightlyshaders_blur_x11_SOURCES})

target_include_directories(lightlyshaders_blur_x11 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(lightlyshaders_blur_x11 PRIVATE
    KWinX11::kwin
    KF6::ConfigGui
    PRIVATE XCB::XCB
    KDecoration3::KDecoration
    lshelper
)

install(TARGETS lightlyshaders_blur_x11 DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin-x11/effects/plugins)
