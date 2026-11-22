// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    property var articleViewPage: null
    
    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // In multi-column mode, give the PageRow a preferred column width.
    // In 1-pane (SingleColumn) mode, let the ColumnView fill all available space.
    implicitWidth: (applicationWindow() && applicationWindow().layoutMode === 1) ? 0 : (settings ? settings.centerPaneWidth : 400)
    
    title: articleListModel.currentName

    function fontWithWeight(f, weight) {
        if (!f) return Qt.font({ weight: weight });
        let spec = {
            family: f.family,
            weight: weight,
            italic: f.italic
        };
        if (f.pixelSize > 0) {
            spec.pixelSize = f.pixelSize;
        } else if (f.pointSize > 0) {
            spec.pointSize = f.pointSize;
        }
        return Qt.font(spec);
    }

    actions: [
        Kirigami.Action {
            text: i18n("Mark All Read")
            icon.name: "mail-mark-read"
            onTriggered: articleListModel.markAllAsRead()
            visible: articleListModel.count > 0
            displayHint: (settings && settings.showToolbarLabels) ? Kirigami.DisplayHint.NoPreference : Kirigami.DisplayHint.IconOnly
        },
        Kirigami.Action {
            id: filterAction
            text: articleListModel.filterMode === "starred" ? i18n("Starred") : (articleListModel.filterMode === "unread" ? i18n("Unread") : i18n("All"))
            displayHint: (settings && settings.showToolbarLabels) ? Kirigami.DisplayHint.NoPreference : Kirigami.DisplayHint.IconOnly
            icon.name: articleListModel.filterMode === "starred" ? "favorite" : (articleListModel.filterMode === "unread" ? "mail-unread" : "view-list-details")
            Kirigami.Action {
                text: i18n("All")
                icon.name: "view-list-details"
                checkable: true
                checked: articleListModel.filterMode === "all"
                onTriggered: articleListModel.filterMode = "all"
            }
            Kirigami.Action {
                text: i18n("Unread")
                icon.name: "mail-unread"
                checkable: true
                checked: articleListModel.filterMode === "unread"
                onTriggered: articleListModel.filterMode = "unread"
            }
            Kirigami.Action {
                text: i18n("Starred")
                icon.name: "favorite"
                checkable: true
                checked: articleListModel.filterMode === "starred"
                onTriggered: articleListModel.filterMode = "starred"
            }
        }
    ]

    // Function to select and open an article (reused by click and shortcuts)
    function openArticle(index) {
        if (index < 0 || index >= articleListModel.count) return;
        
        articleListView.currentIndex = index
        let model = articleListModel.get(index);
        root.articleViewPage.loadArticle(model);

        if (applicationWindow().pageStack.depth < 2) {
            applicationWindow().pageStack.push(root.articleViewPage);
        } else if (!applicationWindow().wideScreen) {
            applicationWindow().pageStack.currentIndex = 1;
        }

        // Automatic Read Marking: On Open
        if (settings.markAsReadOnOpen && model.status === "unread") {
            articleListModel.markAsRead(index, true);
        }
    }

    Shortcut {
        sequence: "J"
        onActivated: {
            let nextIndex = articleListView.currentIndex + 1;
            if (nextIndex < articleListModel.count) {
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
            articleListView.currentIndex = -1;
            articleListView.positionViewAtIndex(0, ListView.Beginning);
            
            if (settings && settings.closeReadingPaneOnReset) {
                if (applicationWindow().pageStack.depth > 1) {
                    applicationWindow().pageStack.pop();
                }
                if (root.articleViewPage) {
                    root.articleViewPage.clearArticle();
                }
            } else {
                root.updateOpenArticle();
            }
        }
        function onDataChanged(topLeft, bottomRight) {
            if (!root.articleViewPage || root.articleViewPage.articleId === null) return;
            for (let r = topLeft.row; r <= bottomRight.row; ++r) {
                if (articleListModel.idAt(r) === root.articleViewPage.articleId) {
                    root.updateOpenArticle();
                    break;
                }
            }
        }
    }

    function updateOpenArticle() {
        if (root.articleViewPage && root.articleViewPage.articleId !== null) {
            let modelData = articleListModel.getById(root.articleViewPage.articleId);
            if (modelData && modelData.id !== undefined) {
                root.articleViewPage.syncUpdates(modelData);
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
                model: articleListModel
                currentIndex: -1
                highlightFollowsCurrentItem: true
                boundsBehavior: Flickable.StopAtBounds

                onContentYChanged: {
                    if (!settings.markAsReadOnScroll) return;
                    if (contentY <= 0) return;
                    if (!articleListView.dragging && !articleListView.flicking) return;
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
                            articleListModel.markReadUpTo(topIndex);
                        }
                    }
                }

                delegate: QQC2.ItemDelegate {
                    id: delegate
                    width: ListView.view.width
                    highlighted: ListView.isCurrentItem

                    QQC2.ToolTip.visible: hovered && titleLabel.truncated
                    QQC2.ToolTip.text: model.title
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    
                    onClicked: root.openArticle(index)

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.mediumSpacing

                        // 1. Prefix: Unread dot and Feed Icon
                        RowLayout {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 2
                            spacing: Kirigami.Units.smallSpacing

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.highlightColor
                                opacity: model.status === "unread" ? 1 : 0
                                Layout.alignment: Qt.AlignVCenter
                            }
                            
                            Kirigami.Icon {
                                source: model.feedHasIcon ? ("image://kiriflux/feed-icon/" + model.feedId + "?v=" + model.feedIconId) : "rss"
                                fallback: "rss"
                                isMask: !model.feedHasIcon
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        // 2. Content: Title and Metadata (Aligned)
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            QQC2.Label {
                                id: titleLabel
                                text: model.title
                                textFormat: Text.PlainText
                                font: root.fontWithWeight(
                                    settings ? settings.articleListTitleFont : null,
                                    model.status === "unread"
                                        ? Font.Bold
                                        : (settings && settings.articleListTitleFont ? settings.articleListTitleFont.weight : Font.Normal))
                                elide: Text.ElideRight
                                maximumLineCount: settings ? settings.titleLines : 1
                                wrapMode: (settings && settings.titleLines > 1) ? Text.WordWrap : Text.NoWrap
                                Layout.fillWidth: true
                                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }

                            QQC2.Label {
                                text: {
                                    let parts = [];
                                    if (articleListModel.currentFeedId === -1 && model.feedTitle)
                                        parts.push(model.feedTitle);
                                    if (model.author && model.author !== model.feedTitle)
                                        parts.push(model.author);
                                    if (model.publishedAt)
                                        parts.push(new Date(model.publishedAt).toLocaleString(Qt.locale(), "MMM d, hh:mm"));
                                    return parts.join(" • ");
                                }
                                font: settings.articleListInfoFont
                                opacity: delegate.highlighted ? 0.85 : 0.65
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.largeSpacing * 4
                    visible: articleListView.count === 0
                    icon.name: articleListModel.filterMode === "starred" ? "favorite" : "mail-mark-read"
                    text: articleListModel.filterMode === "starred"
                        ? i18n("No starred articles")
                        : (articleListModel.filterMode === "unread" ? i18n("No unread articles") : i18n("No articles"))
                }
            }
        }
    }
}
