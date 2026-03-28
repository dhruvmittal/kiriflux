find_package(KF6Config REQUIRED)

add_subdirectory(core)

# ── Kiriflux executable ──────────────────────────────────────────────
 
add_executable(kiriflux
    main.cpp
)

target_compile_definitions(kiriflux PRIVATE
    KIRIFLUX_DATA_DIR="${KDE_INSTALL_FULL_DATADIR}/kiriflux"
)

target_link_libraries(kiriflux PRIVATE
    kiriflux_core
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::XmlGui
    KF6::DBusAddons
    Qt6::DBus
)

# ── Install ──────────────────────────────────────────────────────────
set(QML_INSTALL_DIR "${KDE_INSTALL_DATADIR}/kiriflux/qml")
file(COPY Main.qml SettingsPage.qml DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
install(FILES Main.qml SettingsPage.qml DESTINATION ${QML_INSTALL_DIR})
install(FILES ../org.kde.kiriflux.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS kiriflux ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
