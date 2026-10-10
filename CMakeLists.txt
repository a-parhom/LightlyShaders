cmake_minimum_required(VERSION 3.16.0)

project(lightlyshaders VERSION 3.0.0)

set(QT_MAJOR_VERSION 6)
set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG_OUTPUT")

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)

# KWin's effect factory macros declare a Q_OBJECT class, let AutoMoc know
list(APPEND CMAKE_AUTOMOC_MACRO_NAMES
    KWIN_EFFECT_FACTORY_SUPPORTED
    KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED
)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Gui
    Widgets
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    CoreAddons
    I18n
    KCMUtils
    WidgetsAddons
)

find_package(XCB REQUIRED COMPONENTS XCB)

find_package(KWin 6.7 REQUIRED COMPONENTS
    kwineffects
)

find_package(KDecoration3 REQUIRED)

find_package(KWinDBusInterface CONFIG REQUIRED)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# "make clang-format" formats our own sources, src/blur is kept as close to KWin's blur as possible
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/lightlyshaders/*.cpp src/lightlyshaders/*.h src/liblshelper/*.cpp src/liblshelper/*.h tests/*.cpp)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
