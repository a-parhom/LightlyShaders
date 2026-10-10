add_subdirectory(kcm)

add_library(kwin_effect_lightlyshaders MODULE
    lightlyshaders.cpp
    lightlyshaders.qrc
)

target_link_libraries(kwin_effect_lightlyshaders PRIVATE
    lshelper
)

install(TARGETS kwin_effect_lightlyshaders DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins)
