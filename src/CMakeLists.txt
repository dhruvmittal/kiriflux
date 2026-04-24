find_package(KF6Config REQUIRED)

add_subdirectory(core)

# ── Kiriflux executable ──────────────────────────────────────────────
 
qt_add_executable(kiriflux
    main.cpp
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
    kiriflux_core
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
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

# On NixOS, QML modules (Kirigami, etc.) are found at runtime via QML2_IMPORT_PATH.
# Linking KF6::Kirigami triggers qt6_import_qml_plugins which fails because plugin
# targets live in separate Nix store paths. On standard distros (Arch, Fedora, etc.),
# linking KF6::Kirigami directly is needed for the QML engine to find the module.
if(NOT NIX_BUILD)
    target_link_libraries(kiriflux PRIVATE KF6::Kirigami)
endif()

# Handle KConfigXT generated files for the main target - handled by kiriflux_core

# ── Install ──────────────────────────────────────────────────────────
install(FILES ../org.kde.kiriflux.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ../icons/hicolor/scalable/apps/org.kde.kiriflux.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(TARGETS kiriflux DESTINATION ${KDE_INSTALL_BINDIR})
