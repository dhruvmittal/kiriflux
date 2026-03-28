// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kiriflux 1.0
import QtWebEngine

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

    ArticleListModel {
        id: articleListModel
    }

    Connections {
        target: syncEngine
        function onArticlesUpdated() {
            articleListModel.refresh();
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
    menuBar: QQC2.MenuBar {
        QQC2.Menu {
            title: i18n("&File")
            QQC2.Action {
                text: i18n("&Sync Now")
                icon.name: "view-refresh"
                shortcut: "Ctrl+R"
                onTriggered: syncEngine.forceSync()
            }
            QQC2.MenuSeparator {}
            QQC2.Action {
                text: i18n("&Quit")
                icon.name: "application-exit"
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        }

        QQC2.Menu {
            title: i18n("&Edit")
            QQC2.Action {
                text: i18n("&Preferences…")
                icon.name: "settings-configure"
                shortcut: "Ctrl+,"
                onTriggered: settingsDialog.open()
            }
        }

        QQC2.Menu {
            title: i18n("&View")
            QQC2.Action {
                text: i18n("&All Articles")
                icon.name: "view-list-details"
                onTriggered: {
                    articleListModel.currentFeedId = -1;
                    feedListView.currentIndex = -1;
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
        title: "Kiriflux"
        padding: 0

        // No page-level actions — everything is in the menu bar
        Kirigami.Theme.colorSet: Kirigami.Theme.View

        QQC2.SplitView {
            anchors.fill: parent
            orientation: Qt.Horizontal

            // ── Pane 1: Feeds List ────────────────────────────────
            QQC2.Pane {
                QQC2.SplitView.preferredWidth: 250
                QQC2.SplitView.minimumWidth: 200
                padding: 0
                
                Kirigami.Theme.colorSet: Kirigami.Theme.Window

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Left Pane Toolbar
                    QQC2.ToolBar {
                        Layout.fillWidth: true
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.smallSpacing
                            anchors.rightMargin: Kirigami.Units.smallSpacing
                            
                            QQC2.Label {
                                text: syncEngine.lastSyncTimeText
                                font.pixelSize: 10
                                opacity: 0.6
                                Layout.fillWidth: true
                            }

                            QQC2.ToolButton {
                                icon.name: "view-refresh"
                                text: i18n("Refresh")
                                display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                onClicked: syncEngine.forceSync()
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.visible: hovered
                            }
                        }
                    }

                    ListView {
                        id: feedListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: feedListModel
                        currentIndex: -1

                        header: QQC2.ItemDelegate {
                            width: feedListView.width
                            highlighted: articleListModel.currentFeedId === -1
                            text: i18n("All Articles")
                            icon.name: "view-list-details"
                            onClicked: {
                                articleListModel.currentFeedId = -1;
                                feedListView.currentIndex = -1;
                            }
                        }

                        delegate: QQC2.ItemDelegate {
                            width: feedListView.width
                            highlighted: ListView.isCurrentItem
                            onClicked: {
                                feedListView.currentIndex = index;
                                articleListModel.currentFeedId = model.id;
                            }
                            
                            contentItem: RowLayout {
                                Kirigami.Icon {
                                    source: "rss" 
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
                                    implicitWidth: Math.max(20, countLabel.implicitWidth + 8)
                                    implicitHeight: 16
                                    radius: 8
                                    color: Kirigami.Theme.highlightColor
                                    QQC2.Label {
                                        id: countLabel
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

            // ── Pane 2: Article List ───────────────────────────────
            QQC2.Pane {
                QQC2.SplitView.preferredWidth: 350
                QQC2.SplitView.minimumWidth: 250
                padding: 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Center Pane Toolbar
                    QQC2.ToolBar {
                        Layout.fillWidth: true
                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Item { Layout.fillWidth: true } // Left Spacer

                            QQC2.ToolButton {
                                icon.name: "mail-mark-read"
                                text: i18n("Mark All Read")
                                display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                onClicked: articleListModel.markAllAsRead()
                                QQC2.ToolTip.text: text
                            }

                            QQC2.ToolSeparator {}

                            QQC2.ToolButton {
                                checkable: true
                                checked: articleListModel.filterStatus === "unread" && !articleListModel.filterStarred
                                icon.name: "mail-unread"
                                text: i18n("Unread")
                                display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                onClicked: {
                                    articleListModel.filterStatus = "unread";
                                    articleListModel.filterStarred = false;
                                }
                                QQC2.ToolTip.text: text
                            }

                            QQC2.ToolButton {
                                checkable: true
                                checked: articleListModel.filterStarred
                                icon.name: "favorite"
                                text: i18n("Starred")
                                display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                onClicked: articleListModel.filterStarred = true
                                QQC2.ToolTip.text: text
                            }

                            QQC2.ToolButton {
                                checkable: true
                                checked: articleListModel.filterStatus === "all" && !articleListModel.filterStarred
                                icon.name: "view-list-details"
                                text: i18n("All")
                                display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                onClicked: {
                                    articleListModel.filterStatus = "all";
                                    articleListModel.filterStarred = false;
                                }
                                QQC2.ToolTip.text: text
                            }

                            Item { Layout.fillWidth: true } // Right Spacer
                        }
                    }

                    // Sync Status Indicator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
                        color: Kirigami.Theme.highlightColor
                        opacity: 0.1
                        visible: syncEngine.isSyncing
                        
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: Kirigami.Units.smallSpacing
                            QQC2.BusyIndicator {
                                implicitWidth: Kirigami.Units.gridUnit
                                implicitHeight: Kirigami.Units.gridUnit
                                running: syncEngine.isSyncing
                            }
                            QQC2.Label {
                                text: i18n("Syncing...")
                                font.italic: true
                                font.pixelSize: Kirigami.Units.gridUnit * 0.8
                            }
                        }
                    }

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
                QQC2.SplitView.fillWidth: true
                QQC2.SplitView.minimumWidth: 400
                padding: 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Right Pane Toolbar (unchanged ...)
                    QQC2.ToolBar {
                        Layout.fillWidth: true
                        visible: articleListView.currentIndex >= 0
                        RowLayout {
                             anchors.fill: parent
                             spacing: 0
                             Item { Layout.fillWidth: true } // Left Spacer
                             QQC2.ToolButton {
                                 icon.name: articleListView.currentItem && articleListView.currentItem.isUnread ? "mail-mark-read" : "mail-mark-unread"
                                 text: articleListView.currentItem && articleListView.currentItem.isUnread ? i18n("Mark Read") : i18n("Mark Unread")
                                 display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                 onClicked: articleListModel.markAsRead(articleListView.currentIndex, articleListView.currentItem.isUnread)
                                 QQC2.ToolTip.text: text
                             }
                             QQC2.ToolButton {
                                 icon.name: "favorite"
                                 text: i18n("Star")
                                 display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                 onClicked: articleListModel.toggleStarred(articleListView.currentIndex)
                                 QQC2.ToolTip.text: text
                             }
                             QQC2.ToolButton {
                                 icon.name: "document-export"
                                 text: i18n("Save to Service")
                                 display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                 onClicked: syncEngine.saveToService(articleListModel.data(articleListModel.index(articleListView.currentIndex, 0), ArticleListModel.IdRole))
                                 QQC2.ToolTip.text: text
                             }
                             QQC2.ToolButton {
                                 icon.name: "document-import"
                                 text: i18n("Fetch Text")
                                 display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                 onClicked: syncEngine.fetchFullContent(articleListModel.data(articleListModel.index(articleListView.currentIndex, 0), ArticleListModel.IdRole))
                                 QQC2.ToolTip.text: text
                             }
                             QQC2.ToolButton {
                                 icon.name: "internet-services"
                                 text: i18n("Open Browser")
                                 display: syncEngine.showToolbarLabels ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                                 onClicked: Qt.openUrlExternally(articleListModel.data(articleListModel.index(articleListView.currentIndex, 0), ArticleListModel.UrlRole))
                                 QQC2.ToolTip.text: text
                             }
                        }
                    }

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
