# ── Kiriflux Core Library ──────────────────────────────────────────

set(kiriflux_core_SRCS
    AppController.cpp
    DatabaseManager.cpp
    MinifluxClient.cpp
    SyncEngine.cpp
    FeedListModel.cpp
    ArticleListModel.cpp
    CategoryListModel.cpp
    KirifluxLogging.cpp
    NetworkManager.cpp
)

# KConfigXT setup
kconfig_add_kcfg_files(kiriflux_core_SRCS kiriflux.kcfgc GENERATE_MOC)

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
