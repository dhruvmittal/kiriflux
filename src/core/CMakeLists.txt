# ── Kiriflux Core Library ──────────────────────────────────────────

find_package(KF6Config REQUIRED)

set(kiriflux_core_SRCS
    DatabaseManager.cpp
    MinifluxClient.cpp
    SyncEngine.cpp
    FeedListModel.cpp
    ArticleListModel.cpp
    CategoryListModel.cpp
)

# KConfigXT setup 
kconfig_add_kcfg_files(kiriflux_core_SRCS kiriflux.kcfgc)

# We define this as a regular library for tests to link against.
# The main app will compile these sources directly into its QML module.
add_library(kiriflux_core STATIC ${kiriflux_core_SRCS})

target_link_libraries(kiriflux_core PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::Qml
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
)

target_include_directories(kiriflux_core PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
