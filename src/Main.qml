// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import Qt.labs.platform as Platform

import "ui"

Kirigami.ApplicationWindow {
    id: root

    title: "Kiriflux"
    width: 1200
    height: 800
    minimumWidth: 900
    minimumHeight: 600

    // ── Preferences Dialog ─────────────────────────────────────────
    SettingsPage {
        id: settingsDialog
    }

    Connections {
        target: syncEngine
        function onArticlesUpdated() {
            articleListModel.refresh();
            feedListModel.refresh();
            categoryListModel.refresh();
        }
        function onArticleStatusUpdated(entryId, status) {
            if (status === "read") {
                // Optional: show notification for manual read marks? 
                // Usually we don't notify for every read mark, but for "saved" we did.
                // Keeping it quiet for now unless it was a specific 'save' action.
            }
        }
        function onArticleBookmarkToggled(entryId) {
             root.showPassiveNotification(i18n("Article star status updated."));
        }
    }

    Component.onCompleted: {
        articleListModel.refresh();
        feedListModel.refresh();
        categoryListModel.refresh();
    }

    // ── Layout Control Properties ──────────────────────────────────
    // 3 columns: Drawer pinned (modal=false) + pageStack wide
    // 2 columns: Drawer collapsed (modal=true) + pageStack wide
    // 1 column: Drawer collapsed (modal=true) + pageStack narrow
    property int layoutMode: 3 // Default to 3-column

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton

    // Force pageStack column mode based on layout selection
    // In Kirigami, wideScreen true allows side-by-side pages.
    wideScreen: layoutMode !== 1

    // Configuration for Kirigami PageRow (columnView)
    // We let Kirigami handle column widths automatically for smoother transitions
    property var columnViewSettings: pageStack.columnView

    // ── Layout Menu ────────────────────────────────────────────────
    Platform.MenuBar {
        id: globalMenuBar

        Platform.Menu {
            title: i18n("&File")
            Platform.MenuItem {
                text: i18n("&Sync Now")
                icon.name: "view-refresh"
                shortcut: "Ctrl+R"
                onTriggered: syncEngine.forceSync()
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: i18n("&Quit")
                icon.name: "application-exit"
                shortcut: "Ctrl+Q"
                onTriggered: Qt.quit()
            }
        }

        Platform.Menu {
            title: i18n("&Edit")
            Platform.MenuItem {
                text: i18n("&Preferences…")
                icon.name: "settings-configure"
                shortcut: "Ctrl+,"
                onTriggered: settingsDialog.open()
            }
        }

        Platform.Menu {
            title: i18n("&View")

            Platform.MenuItem {
                text: i18n("3-Column Layout (Pinned Drawer)")
                checkable: true
                checked: root.layoutMode === 3
                onTriggered: root.layoutMode = 3
            }
            Platform.MenuItem {
                text: i18n("2-Column Layout (Collapsible Drawer)")
                checkable: true
                checked: root.layoutMode === 2
                onTriggered: root.layoutMode = 2
            }
            Platform.MenuItem {
                text: i18n("1-Column Layout (Mobile Style)")
                checkable: true
                checked: root.layoutMode === 1
                onTriggered: root.layoutMode = 1
            }

            Platform.MenuSeparator {}
            
            Platform.Menu {
                title: i18n("Title Lines")
                Platform.MenuItem {
                    text: i18n("1 Line")
                    checkable: true
                    checked: syncEngine.titleLines === 1
                    onTriggered: syncEngine.titleLines = 1
                }
                Platform.MenuItem {
                    text: i18n("2 Lines")
                    checkable: true
                    checked: syncEngine.titleLines === 2
                    onTriggered: syncEngine.titleLines = 2
                }
                Platform.MenuItem {
                    text: i18n("3 Lines")
                    checkable: true
                    checked: syncEngine.titleLines === 3
                    onTriggered: syncEngine.titleLines = 3
                }
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: i18n("Use Native Reader Mode")
                checkable: true
                checked: syncEngine.useReaderMode
                onTriggered: syncEngine.useReaderMode = !syncEngine.useReaderMode
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: i18n("&All Articles")
                icon.name: "view-list-details"
                onTriggered: {
                    articleListModel.currentFeedId = -1;
                    articleListModel.currentCategoryId = -1;
                }
            }
        }
    }

    // ── Global Drawer (Left Pane: Feeds) ───────────────────────────
    globalDrawer: FeedsDrawer {
        // Inherit dynamic layout mode
        modal: root.layoutMode !== 3 || Kirigami.Settings.isMobile
    }

    // ── Global Keyboard Shortcuts ──────────────────────────────────
    // Assuming article listView is inside the active page now, we handle shortcuts contextually
    Shortcut {
        sequence: "R"
        onActivated: syncEngine.forceSync()
    }

    // ── Pre-instantiated Pages ─────────────────────────────────────
    ArticleListPage {
        id: articleListPage
    }

    // ── Main Content (Middle Pane: Article List) ───────────────────
    pageStack.initialPage: articleListPage

    // Guaranteed visible progress bar overlay
    Item {
        parent: root.overlay
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        z: 99999
        visible: syncEngine.isSyncing

        Rectangle {
            id: progressIndicator
            height: parent.height
            width: parent.width * 0.3
            color: Kirigami.Theme.highlightColor
            y: 0
            
            NumberAnimation on x {
                loops: Animation.Infinite
                from: -progressIndicator.width
                to: root.width
                duration: 1000
                running: syncEngine.isSyncing
            }
        }
    }
}
