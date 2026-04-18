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
    minimumWidth: layoutMode === 1 ? 320 : 900
    minimumHeight: layoutMode === 1 ? 400 : 600

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
        function onSyncError(msg) {
            root.showPassiveNotification(i18n("Sync error: %1", msg));
        }
    }

    Component.onCompleted: {
        articleListModel.refresh();
        feedListModel.refresh();
        categoryListModel.refresh();
        // Deferred second refresh ensures the article list is populated after the
        // QML engine finishes component instantiation (avoids blank center pane on cold launch).
        startupRefreshTimer.start();
    }

    Timer {
        id: startupRefreshTimer
        interval: 150
        repeat: false
        onTriggered: articleListModel.refresh()
    }

    // ── Layout Control Properties ──────────────────────────────────
    // 3 columns: Drawer pinned (modal=false) + pageStack wide
    // 2 columns: Drawer modal (closeable) + pageStack wide
    // 1 column:  Drawer modal + pageStack narrow (one page at a time)
    property int layoutMode: 3 // Default to 3-column

    onLayoutModeChanged: {
        if (layoutMode === 3) {
            globalDrawer.modal = false;
            globalDrawer.drawerOpen = true;
            // Allow multiple columns side-by-side
            if (pageStack.columnView) {
                pageStack.columnView.columnResizeMode = Kirigami.ColumnView.FixedColumns;
            }
        } else if (layoutMode === 2) {
            globalDrawer.modal = true;
            globalDrawer.drawerOpen = false;
            // Allow multiple columns side-by-side (drawer is just hidden)
            if (pageStack.columnView) {
                pageStack.columnView.columnResizeMode = Kirigami.ColumnView.FixedColumns;
            }
        } else {
            // wideScreen alone is not enough — the PageRow fits columns based on
            // available pixel width and ignores wideScreen for column count.
            globalDrawer.modal = true;
            globalDrawer.drawerOpen = false;
            if (pageStack.columnView) {
                pageStack.columnView.columnResizeMode = Kirigami.ColumnView.SingleColumn;
            }
        }
    }

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton

    // wideScreen controls back-button visibility and drawer modal default.
    // The actual page stack column count is driven imperatively above.
    wideScreen: layoutMode !== 1

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
                    checked: settings.titleLines === 1
                    onTriggered: settings.titleLines = 1
                }
                Platform.MenuItem {
                    text: i18n("2 Lines")
                    checkable: true
                    checked: settings.titleLines === 2
                    onTriggered: settings.titleLines = 2
                }
                Platform.MenuItem {
                    text: i18n("3 Lines")
                    checkable: true
                    checked: settings.titleLines === 3
                    onTriggered: settings.titleLines = 3
                }
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: i18n("Use Native Reader Mode")
                checkable: true
                checked: settings.useReaderMode
                onTriggered: settings.useReaderMode = !settings.useReaderMode
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
        // Initial state: pinned open for 3-pane, closed for 2/1-pane.
        // The onLayoutModeChanged handler in root drives subsequent changes.
        modal: root.layoutMode !== 3 || Kirigami.Settings.isMobile
        drawerOpen: root.layoutMode === 3 && !Kirigami.Settings.isMobile
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
