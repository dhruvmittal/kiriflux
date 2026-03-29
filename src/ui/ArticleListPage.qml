// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    
    // Dynamically set title based on the selected feed/category
    title: {
        if (articleListModel.currentFeedId !== -1) {
            return i18n("Feed Articles")
        } else if (articleListModel.currentCategoryId !== -1) {
            return i18n("Category Articles")
        }
        return i18n("All Articles")
    }

    actions: [
        Kirigami.Action {
            text: i18n("Mark All Read")
            icon.name: "mail-mark-read"
            onTriggered: articleListModel.markAllAsRead()
            visible: articleListModel.rowCount() > 0
        },
        Kirigami.Action {
            id: filterAction
            text: i18n("Filter")
            icon.name: articleListModel.filterStarred ? "favorite" : (articleListModel.filterStatus === "unread" ? "mail-unread" : "view-list-details")
            Kirigami.Action {
                text: i18n("All")
                icon.name: "view-list-details"
                checkable: true
                checked: articleListModel.filterStatus === "all" && !articleListModel.filterStarred
                onTriggered: {
                    articleListModel.filterStatus = "all";
                    articleListModel.filterStarred = false;
                }
            }
            Kirigami.Action {
                text: i18n("Unread")
                icon.name: "mail-unread"
                checkable: true
                checked: articleListModel.filterStatus === "unread" && !articleListModel.filterStarred
                onTriggered: {
                    articleListModel.filterStatus = "unread";
                    articleListModel.filterStarred = false;
                }
            }
            Kirigami.Action {
                text: i18n("Starred")
                icon.name: "favorite"
                checkable: true
                checked: articleListModel.filterStarred
                onTriggered: articleListModel.filterStarred = true
            }
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.TextField {
            id: searchField
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            placeholderText: i18n("Search articles…")
        }

        ListView {
            id: articleListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: articleListModel
            currentIndex: -1
            highlightFollowsCurrentItem: true

            delegate: QQC2.ItemDelegate {
                id: delegate
                width: articleListView.width
                highlighted: ListView.isCurrentItem
                
                onClicked: {
                    articleListView.currentIndex = index
                    // Push the Article View Page onto the stack
                    applicationWindow().pageStack.push(Qt.resolvedUrl("ArticleViewPage.qml"), {
                        articleIndex: index,
                        articleTitle: model.title,
                        articleContent: model.content,
                        feedSiteUrl: model.feedSiteUrl,
                        feedTitle: model.feedTitle,
                        author: model.author,
                        publishedAt: model.publishedAt,
                        isUnread: model.status === "unread",
                        articleUrl: model.url,
                        articleId: model.id
                    });
                }

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
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
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
