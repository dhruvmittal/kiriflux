find_package(KF6Config REQUIRED)
find_package(KF6Kirigami REQUIRED)

add_subdirectory(core)

# ── Kiriflux executable ──────────────────────────────────────────────

# Use kiriflux_exe as the target name to avoid collision with the 'kiriflux' QML namespace
# and its build-time directory. The output binary will still be named 'kiriflux'.
qt_add_executable(kiriflux_exe main.cpp)
set_target_properties(kiriflux_exe PROPERTIES OUTPUT_NAME "kiriflux")

# Use qt_add_qml_module for modern QML resource management and plugin dependency resolution.
# This ensures KF6Kirigami and other dependencies are correctly found and linked.
# We use org.kde.kiriflux to follow KDE standards and avoid local directory naming conflicts.
qt_add_qml_module(kiriflux_exe
    URI org.kde.kiriflux
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
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

target_link_libraries(kiriflux_exe PRIVATE
    kiriflux_core
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
    KF6::Kirigami
)

# Handle KConfigXT generated files for the main target
kconfig_add_kcfg_files(kiriflux_SRCS core/kiriflux.kcfgc)
target_sources(kiriflux_exe PRIVATE ${kiriflux_SRCS})

# ── Install ──────────────────────────────────────────────────────────
install(FILES ../org.kde.kiriflux.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ../icons/hicolor/scalable/apps/org.kde.kiriflux.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(TARGETS kiriflux DESTINATION ${KDE_INSTALL_BINDIR})
