find_package(KF6Config REQUIRED)

add_subdirectory(core)

# ── Kiriflux executable ──────────────────────────────────────────────
 
qt_add_executable(kiriflux
    main.cpp
    core/DatabaseManager.cpp
    core/MinifluxClient.cpp
    core/SyncEngine.cpp
    core/FeedListModel.cpp
    core/ArticleListModel.cpp
    core/CategoryListModel.cpp
)

# Manual resource embedding for guaranteed QRC paths
qt_add_resources(kiriflux "qml"
    PREFIX "/ui/"
    FILES
        Main.qml
        SettingsPage.qml
        ui/FeedsDrawer.qml
        ui/ArticleListPage.qml
        ui/ArticleViewPage.qml
)

qt_add_resources(kiriflux "icons"
    PREFIX "/icons/"
    FILES
        org.kde.kiriflux.svg
)

target_include_directories(kiriflux PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)

target_link_libraries(kiriflux PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::DBus
    Qt6::WebEngineQuick
    KF6::CoreAddons
    KF6::I18n
    KF6::XmlGui
    KF6::DBusAddons
)

# Handle KConfigXT generated files for the main target
kconfig_add_kcfg_files(kiriflux_SRCS core/kiriflux.kcfgc)
target_sources(kiriflux PRIVATE ${kiriflux_SRCS})

# ── Install ──────────────────────────────────────────────────────────
install(FILES ../org.kde.kiriflux.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ../icons/hicolor/scalable/apps/org.kde.kiriflux.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(TARGETS kiriflux DESTINATION ${KDE_INSTALL_BINDIR})
