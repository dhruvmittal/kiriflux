// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.GlobalDrawer {
    id: root

    property bool isMobile: Kirigami.Settings.isMobile

    // By default, pinning it acts as a permanent left pane on desktop
    modal: isMobile
    collapsed: false

    title: "Kiriflux"

    actions: [
        Kirigami.Action {
            text: i18n("Sync")
            icon.name: "view-refresh"
            onTriggered: syncEngine.forceSync()
            tooltip: syncEngine.lastSyncTimeText
        }
    ]

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
                            // Optionally expand when clicked:
                            // categoryItem.expanded = true
                        }

                        contentItem: RowLayout {
                            Kirigami.Icon {
                                source: categoryItem.expanded ? "arrow-down" : "arrow-right"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
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
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
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
