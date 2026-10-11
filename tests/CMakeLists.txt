# Only needed for the tests, so building the effect doesn't depend on them
find_package(Qt6 ${QT_MIN_VERSION} CONFIG QUIET COMPONENTS Test)
find_package(epoxy QUIET)
if(NOT TARGET Qt6::Test OR NOT epoxy_FOUND)
    message(STATUS "Qt6Test or epoxy not found, not building the tests")
    return()
endif()

# Renders the shaders from the source tree on the GPU, skipped when no GPU is available
add_executable(shadertest shadertest.cpp)
target_compile_definitions(shadertest PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(shadertest Qt6::Test epoxy::epoxy)
add_test(NAME shadertest COMMAND shadertest)

set(configtest_SOURCES configtest.cpp)
kconfig_add_kcfg_files(configtest_SOURCES ../src/lightlyshaders/lightlyshaders_config.kcfgc ../src/blur/blurconfig.kcfgc)
add_executable(configtest ${configtest_SOURCES})
target_link_libraries(configtest Qt6::Test KF6::ConfigGui)
add_test(NAME configtest COMMAND configtest)
