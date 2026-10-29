// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.GlobalDrawer {
    id: root

    property bool isMobile: Kirigami.Settings.isMobile
    width: settings ? settings.leftPaneWidth : 250

    // By default, pinning it acts as a permanent left pane on desktop
    modal: isMobile
    collapsed: false

    component UnreadBadge : Rectangle {
        property int count: 0
        property bool highlighted: false
        visible: count > 0
        implicitWidth: Math.max(20, badgeLabel.implicitWidth + 8)
        implicitHeight: 16
        radius: 8
        color: highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.highlightColor
        QQC2.Label {
            id: badgeLabel
            anchors.centerIn: parent
            text: parent.count
            font.pointSize: 9
            font.bold: true
            color: parent.highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.highlightedTextColor
        }
    }

    header: Kirigami.AbstractApplicationHeader {
        minimumHeight: applicationWindow()?.pageStack?.globalToolBar?.minimumHeight ?? (Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing * 2)
        maximumHeight: applicationWindow()?.pageStack?.globalToolBar?.maximumHeight ?? minimumHeight
        preferredHeight: applicationWindow()?.pageStack?.globalToolBar?.preferredHeight ?? minimumHeight
        leftPadding: Kirigami.Units.largeSpacing
        rightPadding: Kirigami.Units.mediumSpacing

        contentItem: RowLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                text: i18n("Kiriflux")
                maximumLineCount: 1
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Kirigami.ActionToolBar {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.fillHeight: true
                alignment: Qt.AlignRight
                actions: [
                    Kirigami.Action {
                        text: i18n("Sync")
                        icon.name: "view-refresh"
                        onTriggered: syncEngine.forceSync()
                        tooltip: syncEngine.lastSyncTimeText
                        displayHint: (settings && settings.showToolbarLabels) ? Kirigami.DisplayHint.NoPreference : Kirigami.DisplayHint.IconOnly
                    }
                ]
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

            // 1. All Articles
            QQC2.ItemDelegate {
                font.family: (settings && settings.leftPaneFont) ? settings.leftPaneFont.family : ""
                font.pointSize: (settings && settings.leftPaneFont && settings.leftPaneFont.pointSize > 0) ? settings.leftPaneFont.pointSize : -1
                font.weight: (settings && settings.leftPaneFont) ? settings.leftPaneFont.weight : Font.Normal
                font.italic: (settings && settings.leftPaneFont) ? settings.leftPaneFont.italic : false
                Layout.fillWidth: true
                highlighted: articleListModel.currentFeedId === -1 && articleListModel.currentCategoryId === -1
                text: i18n("All Articles")
                icon.name: "view-list-details"
                onClicked: {
                    articleListModel.currentFeedId = -1;
                    articleListModel.currentCategoryId = -1;
                }
            }

            // 2. Categories Section
            Kirigami.ListSectionHeader {
                Layout.fillWidth: true
                text: i18n("Categories")
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
                        id: categoryDelegate
                        font.family: (settings && settings.leftPaneFont) ? settings.leftPaneFont.family : ""
                        font.pointSize: (settings && settings.leftPaneFont && settings.leftPaneFont.pointSize > 0) ? settings.leftPaneFont.pointSize : -1
                        font.weight: (settings && settings.leftPaneFont) ? settings.leftPaneFont.weight : Font.Normal
                        font.italic: (settings && settings.leftPaneFont) ? settings.leftPaneFont.italic : false
                        Layout.fillWidth: true
                        highlighted: articleListModel.currentCategoryId === categoryItem.catId
                        onClicked: {
                            articleListModel.currentCategoryId = categoryItem.catId;
                        }

                        contentItem: RowLayout {
                            Kirigami.Icon {
                                source: categoryItem.expanded ? "arrow-down" : "arrow-right"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                selected: categoryDelegate.highlighted
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: (mouse) => {
                                        categoryItem.expanded = !categoryItem.expanded
                                        mouse.accepted = true
                                    }
                                }
                            }
                            QQC2.Label {
                                text: model.title
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.bold: model.unreadCount > 0
                                color: categoryDelegate.highlighted
                                    ? Kirigami.Theme.highlightedTextColor
                                    : Kirigami.Theme.textColor
                            }
                            UnreadBadge {
                                count: model.unreadCount
                                highlighted: categoryDelegate.highlighted
                            }
                        }
                    }

                    // Feeds in this category
                    Repeater {
                        model: feedListModel
                        delegate: QQC2.ItemDelegate {
                            id: nestedFeedDelegate
                            font.family: (settings && settings.leftPaneFont) ? settings.leftPaneFont.family : ""
                            font.pointSize: (settings && settings.leftPaneFont && settings.leftPaneFont.pointSize > 0) ? settings.leftPaneFont.pointSize : -1
                            font.weight: (settings && settings.leftPaneFont) ? settings.leftPaneFont.weight : Font.Normal
                            font.italic: (settings && settings.leftPaneFont) ? settings.leftPaneFont.italic : false
                            Layout.fillWidth: true
                            visible: categoryItem.expanded && model.categoryId === categoryItem.catId
                            height: visible ? implicitHeight : 0
                            Layout.leftMargin: Kirigami.Units.largeSpacing
                            highlighted: articleListModel.currentFeedId === model.id
                            onClicked: articleListModel.currentFeedId = model.id
                            
                            contentItem: RowLayout {
                                QQC2.Label {
                                    text: model.title
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    font.bold: model.unreadCount > 0
                                    color: nestedFeedDelegate.highlighted
                                        ? Kirigami.Theme.highlightedTextColor
                                        : Kirigami.Theme.textColor
                                }
                                UnreadBadge {
                                    count: model.unreadCount
                                    highlighted: nestedFeedDelegate.highlighted
                                }
                            }
                        }
                    }
                }
            }

            // 3. Feeds Section (Flat list)
            Kirigami.ListSectionHeader {
                Layout.fillWidth: true
                text: i18n("Feeds")
            }

            Repeater {
                model: feedListModel
                delegate: QQC2.ItemDelegate {
                    id: flatFeedDelegate
                    font.family: (settings && settings.leftPaneFont) ? settings.leftPaneFont.family : ""
                    font.pointSize: (settings && settings.leftPaneFont && settings.leftPaneFont.pointSize > 0) ? settings.leftPaneFont.pointSize : -1
                    font.weight: (settings && settings.leftPaneFont) ? settings.leftPaneFont.weight : Font.Normal
                    font.italic: (settings && settings.leftPaneFont) ? settings.leftPaneFont.italic : false
                    Layout.fillWidth: true
                    highlighted: articleListModel.currentFeedId === model.id
                    onClicked: articleListModel.currentFeedId = model.id
                    
                    contentItem: RowLayout {
                        Kirigami.Icon {
                            source: model.siteUrl ? "https://www.google.com/s2/favicons?domain=" + model.siteUrl : "rss"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                            selected: flatFeedDelegate.highlighted
                        }
                        QQC2.Label {
                            text: model.title
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            font.bold: model.unreadCount > 0
                            color: flatFeedDelegate.highlighted
                                ? Kirigami.Theme.highlightedTextColor
                                : Kirigami.Theme.textColor
                        }
                        UnreadBadge {
                            count: model.unreadCount
                            highlighted: flatFeedDelegate.highlighted
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
    }
}
