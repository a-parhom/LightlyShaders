find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
find_package(epoxy REQUIRED)

# Renders the shaders from the source tree on the GPU, skipped when no GPU is available
add_executable(shadertest shadertest.cpp)
target_compile_definitions(shadertest PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(shadertest Qt6::Test epoxy::epoxy)
add_test(NAME shadertest COMMAND shadertest)

set(configtest_SOURCES configtest.cpp)
kconfig_add_kcfg_files(configtest_SOURCES ../src/lightlyshaders/lightlyshaders_config.kcfgc)
add_executable(configtest ${configtest_SOURCES})
target_link_libraries(configtest Qt6::Test KF6::ConfigGui)
add_test(NAME configtest COMMAND configtest)
