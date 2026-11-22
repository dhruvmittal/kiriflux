add_subdirectory(core)

# ── Kiriflux executable ──────────────────────────────────────────────
 
qt_add_executable(kiriflux
    main.cpp
    KirifluxImageProvider.cpp
)

# Ahead-of-Time QML compilation and embedding
qt_add_qml_module(kiriflux
    URI org.kde.kiriflux
    QML_FILES
        Main.qml
        SettingsPage.qml
        ui/FeedsDrawer.qml
        ui/ArticleListPage.qml
        ui/ArticleViewPage.qml
)

qt_add_resources(kiriflux "icons"
    PREFIX "/icons"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hicolor/scalable/apps"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hicolor/scalable/apps/org.kde.kiriflux.svg"
)

target_include_directories(kiriflux PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core ${CMAKE_BINARY_DIR})

target_link_libraries(kiriflux PRIVATE
    kiriflux_core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::WebEngineQuick
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
