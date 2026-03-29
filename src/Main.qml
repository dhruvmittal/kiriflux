// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kiriflux 1.0
import QtWebEngine

import Qt.labs.platform as Platform

Kirigami.ApplicationWindow {
    id: root

    title: "Kiriflux"
    width: 1200
    height: 800
    minimumWidth: 900
    minimumHeight: 600

    // ── Models ─────────────────────────────────────────────────────
    FeedListModel {
        id: feedListModel
    }

    CategoryListModel {
        id: categoryListModel
    }

    ArticleListModel {
        id: articleListModel
    }

    Connections {
        target: syncEngine
        function onArticlesUpdated() {
            articleListModel.refresh();
            feedListModel.refresh();
            categoryListModel.refresh();
        }
        function onArticleSavedToService(entryId) {
            root.showPassiveNotification(i18n("Article saved to external services."));
        }
    }

    // ── Preferences Dialog ─────────────────────────────────────────
    SettingsPage {
        id: settingsDialog
    }

    // ── Menu Bar (KDE Global Menu compatible) ─────────────────────
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
                text: i18n("&All Articles")
                icon.name: "view-list-details"
                onTriggered: {
                    articleListModel.currentFeedId = -1;
                    articleListModel.currentCategoryId = -1;
                }
            }
        }
    }

    // ── Global Keyboard Shortcuts ──────────────────────────────────
    Shortcut { sequence: "J"; onActivated: articleListView.incrementCurrentIndex() }
    Shortcut { sequence: "K"; onActivated: articleListView.decrementCurrentIndex() }
    Shortcut {
        sequence: "M"
        onActivated: {
            if (articleListView.currentIndex >= 0) {
                let isUnread = articleListView.currentItem.isUnread;
                articleListModel.markAsRead(articleListView.currentIndex, isUnread);
            }
        }
    }
    Shortcut { sequence: "R"; onActivated: syncEngine.forceSync() }

    // ── Main Content ───────────────────────────────────────────────
    pageStack.initialPage: Kirigami.Page {
        id: mainPage
        padding: 0
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

        Kirigami.Theme.colorSet: Kirigami.Theme.View

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Global Toolbar (aligned with panes below) ────────────
            QQC2.ToolBar {
                Layout.fillWidth: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Section 1: Feeds toolbar
                    Item {
                        implicitWidth: feedPane.width
                        Layout.fillHeight: true
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.largeSpacing
                            anchors.rightMargin: Kirigami.Units.smallSpacing

                            QQC2.Label {
                                text: "Kiriflux"
                                font.bold: true
                                font.pixelSize: Kirigami.Units.gridUnit * 1.1
                            }

                            Item { Layout.fillWidth: true }

                            QQC2.ToolButton {
                                icon.name: "view-refresh"
                                onClicked: syncEngine.forceSync()
                                QQC2.ToolTip.text: syncEngine.lastSyncTimeText
                                QQC2.ToolTip.visible: hovered
                            }
                        }
                    }

                    Kirigami.Separator { Layout.fillHeight: true }

                    // Section 2: Articles toolbar
                    Item {
                        implicitWidth: articlePane.width
                        Layout.fillHeight: true
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.smallSpacing
                            anchors.rightMargin: Kirigami.Units.smallSpacing

                            QQC2.ToolButton {
                                icon.name: "mail-mark-read"
                                onClicked: articleListModel.markAllAsRead()
                                QQC2.ToolTip.text: i18n("Mark All Read")
                                QQC2.ToolTip.visible: hovered
                            }

                            Item { Layout.fillWidth: true }

                            QQC2.ToolButton {
                                property bool active: articleListModel.filterStatus === "unread" && !articleListModel.filterStarred
                                icon.name: "mail-unread"
                                onClicked: {
                                    articleListModel.filterStatus = "unread";
                                    articleListModel.filterStarred = false;
                                }
                                highlighted: active
                                QQC2.ToolTip.text: i18n("Unread Only")
                                QQC2.ToolTip.visible: hovered
                            }
                            QQC2.ToolButton {
                                icon.name: "favorite"
                                onClicked: articleListModel.filterStarred = true
                                highlighted: articleListModel.filterStarred
                                QQC2.ToolTip.text: i18n("Starred Only")
                                QQC2.ToolTip.visible: hovered
                            }
                            QQC2.ToolButton {
                                property bool active: articleListModel.filterStatus === "all" && !articleListModel.filterStarred
                                icon.name: "view-list-details"
                                onClicked: {
                                    articleListModel.filterStatus = "all";
                                    articleListModel.filterStarred = false;
                                }
                                highlighted: active
                                QQC2.ToolTip.text: i18n("All Articles")
                                QQC2.ToolTip.visible: hovered
                            }
                        }
                    }

                    Kirigami.Separator { Layout.fillHeight: true }

                    // Section 3: Article content toolbar
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: articleListView.currentIndex >= 0
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.smallSpacing
                            anchors.rightMargin: Kirigami.Units.largeSpacing

                            QQC2.ToolButton {
                                icon.name: (articleListView.currentItem && articleListView.currentItem.isUnread) ? "mail-mark-read" : "mail-mark-unread"
                                onClicked: articleListModel.markAsRead(articleListView.currentIndex, articleListView.currentItem.isUnread)
                                QQC2.ToolTip.text: (articleListView.currentItem && articleListView.currentItem.isUnread) ? i18n("Mark Read") : i18n("Mark Unread")
                                QQC2.ToolTip.visible: hovered
                            }
                            QQC2.ToolButton {
                                icon.name: "favorite"
                                onClicked: articleListModel.toggleStarred(articleListView.currentIndex)
                                QQC2.ToolTip.text: i18n("Star")
                                QQC2.ToolTip.visible: hovered
                            }

                            Item { Layout.fillWidth: true }

                            QQC2.ToolButton {
                                icon.name: "document-import"
                                onClicked: syncEngine.fetchFullContent(articleListModel.data(articleListModel.index(articleListView.currentIndex, 0), ArticleListModel.IdRole))
                                QQC2.ToolTip.text: i18n("Fetch Full Text")
                                QQC2.ToolTip.visible: hovered
                            }
                            QQC2.ToolButton {
                                icon.name: "internet-services"
                                onClicked: Qt.openUrlExternally(articleListModel.data(articleListModel.index(articleListView.currentIndex, 0), ArticleListModel.UrlRole))
                                QQC2.ToolTip.text: i18n("Open in Browser")
                                QQC2.ToolTip.visible: hovered
                            }
                        }
                    }
                }
            }

            // Sync progress bar
            QQC2.ProgressBar {
                Layout.fillWidth: true
                indeterminate: true
                visible: syncEngine.isSyncing
            }

            // ── Three-pane SplitView ─────────────────────────────────
            QQC2.SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

            // ── Pane 1: Feeds List ────────────────────────────────
            QQC2.Pane {
                id: feedPane
                QQC2.SplitView.preferredWidth: 250
                QQC2.SplitView.minimumWidth: 200
                padding: 0
                
                Kirigami.Theme.colorSet: Kirigami.Theme.Window

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    QQC2.ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: -1
                        clip: true

                        ColumnLayout {
                            width: parent.width
                            spacing: 0

                            // 1. All Articles
                            QQC2.ItemDelegate {
                                Layout.fillWidth: true
                                highlighted: articleListModel.currentFeedId === -1 && articleListModel.currentCategoryId === -1
                                text: i18n("All Articles")
                                icon.name: "view-list-details"
                                onClicked: {
                                    articleListModel.currentFeedId = -1;
                                    articleListModel.currentCategoryId = -1;
                                }
                            }

                            Kirigami.Separator { Layout.fillWidth: true; Layout.margins: Kirigami.Units.smallSpacing }

                            // 2. Categories Section
                            QQC2.Label {
                                text: i18n("Categories")
                                font.bold: true
                                opacity: 0.6
                                font.pixelSize: Kirigami.Units.gridUnit * 0.7
                                Layout.margins: Kirigami.Units.smallSpacing
                            }

                            Repeater {
                                model: categoryListModel
                                delegate: ColumnLayout {
                                    id: categoryItem
                                    Layout.fillWidth: true
                                    spacing: 0
                                    property bool expanded: false
                                    property var catId: model.id

                                    QQC2.ItemDelegate {
                                        Layout.fillWidth: true
                                        highlighted: articleListModel.currentCategoryId === categoryItem.catId
                                        onClicked: {
                                            articleListModel.currentCategoryId = categoryItem.catId;
                                        }

                                        contentItem: RowLayout {
                                            Kirigami.Icon {
                                                source: categoryItem.expanded ? "arrow-down" : "arrow-right"
                                                implicitWidth: 12; implicitHeight: 12
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        categoryItem.expanded = !categoryItem.expanded
                                                        mouse.accepted = true
                                                    }
                                                }
                                            }
                                            QQC2.Label {
                                                text: model.title
                                                Layout.fillWidth: true
                                                elide: Text.ElideRight
                                            }
                                            QQC2.Label {
                                                text: model.unreadCount
                                                visible: model.unreadCount > 0
                                                font.pixelSize: 10
                                                opacity: 0.7
                                            }
                                        }
                                    }

                                    // Feeds in this category
                                    Repeater {
                                        model: feedListModel
                                        delegate: QQC2.ItemDelegate {
                                            Layout.fillWidth: true
                                            visible: categoryItem.expanded && model.categoryId === categoryItem.catId
                                            height: visible ? implicitHeight : 0
                                            Layout.leftMargin: Kirigami.Units.largeSpacing
                                            highlighted: articleListModel.currentFeedId === model.id
                                            text: model.title
                                            onClicked: articleListModel.currentFeedId = model.id
                                        }
                                    }
                                }
                            }

                            Kirigami.Separator { Layout.fillWidth: true; Layout.margins: Kirigami.Units.smallSpacing }

                            // 3. Feeds Section (Flat list)
                            QQC2.Label {
                                text: i18n("Feeds")
                                font.bold: true
                                opacity: 0.6
                                font.pixelSize: Kirigami.Units.gridUnit * 0.7
                                Layout.margins: Kirigami.Units.smallSpacing
                            }

                            Repeater {
                                model: feedListModel
                                delegate: QQC2.ItemDelegate {
                                    Layout.fillWidth: true
                                    highlighted: articleListModel.currentFeedId === model.id
                                    onClicked: articleListModel.currentFeedId = model.id
                                    
                                    contentItem: RowLayout {
                                        Kirigami.Icon {
                                            source: model.siteUrl ? "https://www.google.com/s2/favicons?domain=" + model.siteUrl : "rss"
                                            implicitWidth: 16
                                            implicitHeight: 16
                                        }
                                        QQC2.Label {
                                            text: model.title
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                            font.bold: model.unreadCount > 0
                                        }
                                        Rectangle {
                                            visible: model.unreadCount > 0
                                            implicitWidth: Math.max(20, cLabel.implicitWidth + 8)
                                            implicitHeight: 16
                                            radius: 8
                                            color: Kirigami.Theme.highlightColor
                                            QQC2.Label {
                                                id: cLabel
                                                anchors.centerIn: parent
                                                text: model.unreadCount
                                                font.pixelSize: 10
                                                font.bold: true
                                                color: "white"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Pane 2: Article List ───────────────────────────────
            QQC2.Pane {
                id: articlePane
                QQC2.SplitView.preferredWidth: 350
                QQC2.SplitView.minimumWidth: 250
                padding: 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0


                    QQC2.TextField {
                        id: searchField
                        Layout.fillWidth: true
                        Layout.margins: 8
                        placeholderText: i18n("Search articles…")
                    }

                    ListView {
                        id: articleListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: articleListModel
                        currentIndex: 0
                        highlightFollowsCurrentItem: true

                        delegate: QQC2.ItemDelegate {
                            width: articleListView.width
                            highlighted: ListView.isCurrentItem
                            
                            // Expose data to detail pane and shortcuts
                            property string articleTitle: model.title
                            property string articleContent: model.content
                            property bool isUnread: model.status === "unread"
                            property string feedTitle: model.feedTitle
                            property string feedSiteUrl: model.feedSiteUrl
                            property string author: model.author
                            property string publishedAt: model.publishedAt

                            onClicked: articleListView.currentIndex = index

                            contentItem: ColumnLayout {
                                spacing: 4
                                RowLayout {
                                    spacing: 8
                                    Rectangle {
                                        width: 8; height: 8; radius: 4
                                        color: Kirigami.Theme.highlightColor
                                        visible: model.status === "unread"
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    
                                    Kirigami.Icon {
                                        source: model.feedSiteUrl ? "https://www.google.com/s2/favicons?domain=" + model.feedSiteUrl : "rss"
                                        implicitWidth: 16
                                        implicitHeight: 16
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    QQC2.Label {
                                        text: model.title
                                        font.bold: model.status === "unread"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }
                                QQC2.Label {
                                    text: (model.author ? model.author + " • " : "") + 
                                          (model.publishedAt ? new Date(model.publishedAt).toLocaleTimeString(Qt.locale(), "hh:mm") : "")
                                    font.pixelSize: 11
                                    opacity: 0.6
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 24 // Align with title
                                }
                            }
                        }
                    }
                }
            }

            // ── Pane 3: Article Content ────────────────────────────
            QQC2.Pane {
                id: contentPane
                QQC2.SplitView.fillWidth: true
                QQC2.SplitView.minimumWidth: 400
                padding: 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Kirigami.Page {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            Shortcut {
                                sequence: "Space"
                                onActivated: articleWebView.runJavaScript("window.scrollBy(0, window.innerHeight);")
                            }

                            // Subtitle Area (stays native)
                            Kirigami.AbstractCard {
                                Layout.fillWidth: true
                                visible: articleListView.currentItem !== null
                                contentItem: ColumnLayout {
                                    QQC2.Label {
                                        text: articleListView.currentItem ? articleListView.currentItem.articleTitle : ""
                                        font.pixelSize: 24
                                        font.bold: true
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                    RowLayout {
                                        spacing: 8
                                        Kirigami.Icon {
                                            source: articleListView.currentItem ? "https://www.google.com/s2/favicons?domain=" + articleListView.currentItem.feedSiteUrl : ""
                                            implicitWidth: 16
                                            implicitHeight: 16
                                        }
                                        QQC2.Label {
                                            text: articleListView.currentItem ? 
                                                  (articleListView.currentItem.feedTitle + " • " + 
                                                   (articleListView.currentItem.author ? articleListView.currentItem.author + " • " : "") + 
                                                   new Date(articleListView.currentItem.publishedAt).toLocaleString(Qt.locale())) : ""
                                            font.pixelSize: 11
                                            opacity: 0.7
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            WebEngineView {
                                id: articleWebView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                backgroundColor: Kirigami.Theme.backgroundColor
                                
                                settings.pluginsEnabled: false
                                settings.javascriptEnabled: true
                                
                                // Reset scroll on article change
                                onLoadingChanged: (loadingInfo) => {
                                    if (loadingInfo.status === WebEngineView.LoadSucceededStatus) {
                                        runJavaScript("window.scrollTo(0, 0);")
                                    }
                                }

                                function updateContent() {
                                    if (articleListView.currentItem) {
                                        let html = `
                                            <html>
                                            <head>
                                                <style>
                                                    body {
                                                        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
                                                        padding: 20px;
                                                        line-height: 1.6;
                                                        font-size: 16px;
                                                        color: ${Kirigami.Theme.textColor};
                                                        background-color: ${Kirigami.Theme.backgroundColor};
                                                    }
                                                    img {
                                                        max-width: 100% !important;
                                                        height: auto !important;
                                                        border-radius: 4px;
                                                    }
                                                    pre {
                                                        background-color: #f4f4f4;
                                                        padding: 10px;
                                                        overflow-x: auto;
                                                    }
                                                    a { color: ${Kirigami.Theme.highlightColor}; }
                                                </style>
                                            </head>
                                            <body>
                                                ${articleListView.currentItem.articleContent}
                                            </body>
                                            </html>
                                        `;
                                        loadHtml(html);
                                    } else {
                                        loadHtml("<html><body style='display:flex; justify-content:center; align-items:center; height:90vh; color:#888; font-family:sans-serif;'>Select an article to read</body></html>");
                                    }
                                }

                                Connections {
                                    target: articleListView
                                    function onCurrentIndexChanged() { articleWebView.updateContent(); }
                                }
                                
                                Component.onCompleted: updateContent()
                            }
                        }
                    }
                }
            }
        }
        }
    }
}
