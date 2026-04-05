// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    
    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // Hints for Kirigami.PageRow (column view)
    implicitWidth: syncEngine.centerPaneWidth
    
    title: articleListModel.currentName

    actions: [
        Kirigami.Action {
            text: i18n("Mark All Read")
            icon.name: "mail-mark-read"
            onTriggered: articleListModel.markAllAsRead()
            visible: articleListModel.rowCount() > 0
            displayHint: syncEngine.showToolbarLabels ? 0 : 1
        },
        Kirigami.Action {
            id: filterAction
            text: articleListModel.filterStarred ? i18n("Starred") : (articleListModel.filterStatus === "unread" ? i18n("Unread") : i18n("All"))
            displayHint: syncEngine.showToolbarLabels ? 0 : 1
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

    // Function to select and open an article (reused by click and shortcuts)
    function openArticle(index) {
        if (index < 0 || index >= articleListModel.rowCount()) return;
        
        articleListView.currentIndex = index
        let model = articleListModel.get(index);
        let props = {
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
        };

        // Check if an ArticleViewPage is already on the stack (manual lookup for Kirigami PageRow)
        let page = null;
        for (let i = 0; i < applicationWindow().pageStack.depth; ++i) {
            let item = applicationWindow().pageStack.get(i);
            if (item && item.objectName === "articleViewPage") {
                page = item;
                break;
            }
        }

        if (page) {
            // Update existing page properties
            for (let p in props) {
                page[p] = props[p];
            }
        } else {
            // Push new page
            applicationWindow().pageStack.push(Qt.resolvedUrl("ArticleViewPage.qml"), props);
        }

        // Automatic Read Marking: On Open
        if (syncEngine.markAsReadOnOpen && model.status === "unread") {
            articleListModel.markAsRead(index, true);
        }
    }

    Shortcut {
        sequence: "J"
        onActivated: {
            let nextIndex = articleListView.currentIndex + 1;
            if (nextIndex < articleListModel.rowCount()) {
                root.openArticle(nextIndex);
            }
        }
    }
    Shortcut {
        sequence: "K"
        onActivated: {
            let prevIndex = articleListView.currentIndex - 1;
            if (prevIndex >= 0) {
                root.openArticle(prevIndex);
            }
        }
    }

    Connections {
        target: articleListModel
        function onModelReset() { 
            root.updateOpenArticle(); 
            
            // Reset selection and scroll position
            articleListView.currentIndex = -1;
            articleListView.positionViewAtIndex(0, ListView.Beginning);
            
            // Close any open ArticleViewPage on the stack
            // Search from top to find and pop the article view
            for (let i = applicationWindow().pageStack.depth - 1; i >= 0; --i) {
                let item = applicationWindow().pageStack.get(i);
                if (item && item.objectName === "articleViewPage") {
                    // Pop everything from this page onwards
                    if (i > 0) {
                        applicationWindow().pageStack.pop(applicationWindow().pageStack.get(i-1));
                    } else {
                        applicationWindow().pageStack.pop();
                    }
                    break;
                }
            }
        }
        function onDataChanged() { root.updateOpenArticle(); }
    }

    function updateOpenArticle() {
        for (let i = 0; i < applicationWindow().pageStack.depth; ++i) {
            let item = applicationWindow().pageStack.get(i);
            if (item && item.objectName === "articleViewPage" && item.articleId !== null) {
                let modelData = articleListModel.getById(item.articleId);
                if (modelData) {
                    item.articleContent = modelData.content;
                    item.isUnread = (modelData.status === "unread");
                }
                break;
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            ListView {
                id: articleListView
                width: parent.width
                model: articleListModel
                currentIndex: -1
                highlightFollowsCurrentItem: true
                boundsBehavior: Flickable.StopAtBounds

                onContentYChanged: {
                    if (!syncEngine.markAsReadOnScroll) return;
                    if (contentY <= 0) return;
                    scrollMarkTimer.restart();
                }

                Timer {
                    id: scrollMarkTimer
                    interval: 300
                    repeat: false
                    onTriggered: {
                        let topY = articleListView.contentY;
                        // Try sampling a few pixels down to hit a valid delegate bounds
                        let topIndex = articleListView.indexAt(articleListView.width / 2, topY + 10);
                        if (topIndex === -1) {
                            topIndex = articleListView.indexAt(articleListView.width / 2, topY + 40);
                        }
                        
                        if (topIndex > 0) {
                            // Any item with index < topIndex has scrolled out of view at the top 
                            for (let i = 0; i < topIndex; i++) {
                                let modelItem = articleListModel.get(i);
                                if (modelItem && modelItem.status === "unread") {
                                    articleListModel.markAsRead(i, true);
                                }
                            }
                        }
                    }
                }

                delegate: QQC2.ItemDelegate {
                    id: delegate
                    width: articleListView.width
                    padding: Kirigami.Units.largeSpacing
                    highlighted: ListView.isCurrentItem
                    
                    onClicked: root.openArticle(index)

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        // 1. Prefix: Unread dot and Feed Icon (Fixed Width Gutter)
                        RowLayout {
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.6
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: Kirigami.Units.smallSpacing // Align with first line of title
                            spacing: 8

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: Kirigami.Theme.highlightColor
                                opacity: model.status === "unread" ? 1 : 0
                                Layout.alignment: Qt.AlignVCenter
                            }
                            
                            Kirigami.Icon {
                                source: model.feedSiteUrl ? "https://www.google.com/s2/favicons?domain=" + model.feedSiteUrl : "rss"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        // 2. Content: Title and Metadata (Aligned)
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            QQC2.Label {
                                id: titleLabel
                                text: model.title
                                font: syncEngine.articleListTitleFont
                                elide: Text.ElideRight
                                maximumLineCount: syncEngine.titleLines
                                wrapMode: syncEngine.titleLines > 1 ? Text.WordWrap : Text.NoWrap
                                Layout.fillWidth: true
                                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }
                            Binding {
                                target: titleLabel; property: "font.bold"; value: model.status === "unread"
                            }

                            QQC2.Label {
                                text: (model.author ? model.author + " • " : "") + 
                                      (model.publishedAt ? new Date(model.publishedAt).toLocaleString(Qt.locale(), "MMM d, hh:mm") : "")
                                font: syncEngine.articleListInfoFont
                                opacity: delegate.highlighted ? 1.0 : 0.6
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }
                        }
                    }
                }
            }
        }
    }
}
