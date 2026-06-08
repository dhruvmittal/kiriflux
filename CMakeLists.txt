cmake_minimum_required(VERSION 3.20)

project(kiriflux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Options ──────────────────────────────────────────────────────────
option(BUILD_TESTING "Build the test suite" ON)
option(NIX_BUILD "Building inside a Nix environment (skip static QML plugin import)" OFF)

# ── ECM Setup ────────────────────────────────────────────────────────
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KIRIFLUX
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kiriflux-version.h"
)

# ── Dependencies ─────────────────────────────────────────────────────
set(QT_MAJOR_VERSION 6)
set(QT_VERSION_MAJOR 6)
set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Widgets Sql DBus WebEngineQuick Network Concurrent)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0001 NEW)
find_package(KF6Kirigami REQUIRED)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6Config REQUIRED)
find_package(KF6DBusAddons REQUIRED)
find_package(KF6XmlGui REQUIRED)

# ── Subdirectories ───────────────────────────────────────────────────
add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
