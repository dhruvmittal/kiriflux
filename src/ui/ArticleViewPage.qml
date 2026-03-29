// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import QtWebEngine

Kirigami.Page {
    id: root
    
    // Properties passed when pushing the page
    property int articleIndex: -1
    property string articleTitle: ""
    property string articleContent: ""
    property string feedSiteUrl: ""
    property string feedTitle: ""
    property string author: ""
    property string publishedAt: ""
    property bool isUnread: false
    property string articleUrl: ""
    property var articleId: null

    title: articleTitle

    actions: [
        Kirigami.Action {
            text: root.isUnread ? i18n("Mark Read") : i18n("Mark Unread")
            icon.name: root.isUnread ? "mail-mark-read" : "mail-mark-unread"
            onTriggered: {
                if (root.articleIndex >= 0) {
                    articleListModel.markAsRead(root.articleIndex, root.isUnread)
                    root.isUnread = !root.isUnread
                }
            }
        },
        Kirigami.Action {
            text: i18n("Star")
            icon.name: "favorite"
            onTriggered: {
                if (root.articleIndex >= 0) {
                    articleListModel.toggleStarred(root.articleIndex)
                }
            }
        },
        Kirigami.Action {
            text: i18n("Full Content")
            icon.name: "document-import"
            onTriggered: {
                if (root.articleId !== null) {
                    syncEngine.fetchFullContent(root.articleId)
                }
            }
        },
        Kirigami.Action {
            text: i18n("Open Browser")
            icon.name: "internet-services"
            onTriggered: {
                if (root.articleUrl) {
                    Qt.openUrlExternally(root.articleUrl)
                }
            }
        }
    ]

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
            
            contentItem: ColumnLayout {
                RowLayout {
                    spacing: 8
                    Kirigami.Icon {
                        source: root.feedSiteUrl ? "https://www.google.com/s2/favicons?domain=" + root.feedSiteUrl : ""
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                        visible: root.feedSiteUrl !== ""
                    }
                    QQC2.Label {
                        text: (root.feedTitle ? root.feedTitle : "") + 
                              (root.feedTitle && root.author ? " • " : "") + 
                              (root.author ? root.author : "") + 
                              ((root.feedTitle || root.author) && root.publishedAt ? " • " : "") + 
                              (root.publishedAt ? new Date(root.publishedAt).toLocaleString(Qt.locale()) : "")
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
            
            onLoadingChanged: (loadingInfo) => {
                if (loadingInfo.status === WebEngineView.LoadSucceededStatus) {
                    runJavaScript("window.scrollTo(0, 0);")
                }
            }

            function updateContent() {
                if (root.articleContent) {
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
                            ${root.articleContent}
                        </body>
                        </html>
                    `;
                    loadHtml(html);
                } else {
                    loadHtml("<html><body style='display:flex; justify-content:center; align-items:center; height:90vh; color:#888; font-family:sans-serif;'>No content available.</body></html>");
                }
            }
            
            Component.onCompleted: updateContent()
        }
    }
}
