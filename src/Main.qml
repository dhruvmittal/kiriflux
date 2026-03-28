// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kiriflux 1.0

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
                QQC2.SplitView.preferredWidth: 200
                QQC2.SplitView.minimumWidth: 150
                padding: 0
                
                Kirigami.Theme.colorSet: Kirigami.Theme.Window

                ListView {
                    id: feedListView
                    anchors.fill: parent
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

            // ── Pane 2: Article List ───────────────────────────────
            QQC2.Pane {
                QQC2.SplitView.preferredWidth: 350
                QQC2.SplitView.minimumWidth: 250
                padding: 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

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

                            onClicked: articleListView.currentIndex = index

                            contentItem: ColumnLayout {
                                spacing: 4
                                RowLayout {
                                    Rectangle {
                                        width: 8; height: 8; radius: 4
                                        color: Kirigami.Theme.highlightColor
                                        visible: model.status === "unread"
                                    }
                                    QQC2.Label {
                                        text: model.title
                                        font.bold: model.status === "unread"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                QQC2.Label {
                                    text: model.author 
                                    font.pixelSize: 11
                                    opacity: 0.6
                                    visible: model.author !== ""
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

                Kirigami.Page {
                    anchors.fill: parent
                    
                    QQC2.ScrollView {
                        anchors.fill: parent
                        contentWidth: -1
                        clip: true

                        ColumnLayout {
                            width: Math.min(parent.width - 64, 800)
                            spacing: 16
                            Layout.alignment: Qt.AlignHCenter

                            QQC2.Label {
                                text: articleListView.currentItem ? articleListView.currentItem.articleTitle : ""
                                font.pixelSize: 28
                                font.bold: true
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                Layout.topMargin: 32
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                textFormat: Text.RichText
                                wrapMode: Text.WordWrap
                                font.pixelSize: 16
                                lineHeight: 1.5
                                text: articleListView.currentItem ? articleListView.currentItem.articleContent : i18n("Select an article to read")
                            }
                        }
                    }
                }
            }
        }
    }
}
