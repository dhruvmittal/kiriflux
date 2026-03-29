// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import QtWebEngine

Kirigami.Page {
    id: root
    objectName: "articleViewPage"
    
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

    // Properties for robust color-to-CSS conversion
    readonly property color themedTextColor: Kirigami.Theme.textColor
    readonly property color themedBgColor: Kirigami.Theme.backgroundColor
    readonly property color themedAlternateBgColor: Kirigami.Theme.alternateBackgroundColor
    readonly property color themedHighlightColor: Kirigami.Theme.highlightColor
    readonly property color themedFocusColor: Kirigami.Theme.focusColor

    // Helper to ensure colors are valid CSS strings
    function colorToCss(c) {
        return "rgba(" + Math.floor(c.r*255) + "," + Math.floor(c.g*255) + "," + Math.floor(c.b*255) + "," + c.a + ")";
    }

    onArticleContentChanged: articleWebView.updateContent()
    onArticleTitleChanged: articleWebView.updateContent()
    onThemedTextColorChanged: articleWebView.updateContent()
    onThemedBgColorChanged: articleWebView.updateContent()

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: true

    Component.onCompleted: {
        console.log("ArticleViewPage loaded. Text color:", themedTextColor, "BG color:", themedBgColor);
    }

    // Track whether the WebEngine is ready to render
    property bool webViewReady: false

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

    WebEngineView {
        id: articleWebView
        anchors.fill: parent
        backgroundColor: root.themedBgColor
        
        settings.pluginsEnabled: false
        settings.javascriptEnabled: true
        
        onLoadingChanged: (loadingInfo) => {
            if (loadingInfo.status === WebEngineView.LoadSucceededStatus) {
                runJavaScript("window.scrollTo(0, 0);")
            }
        }

        function updateContent() {
            // Guard: if WebEngine isn't ready yet, schedule a retry
            if (!root.webViewReady) {
                return;
            }

            let textColor = root.themedTextColor;
            let bgColor = root.themedBgColor;
            let alternateBgColor = root.themedAlternateBgColor;
            let highlightColor = root.themedHighlightColor;
            let focusColor = root.themedFocusColor;

            // Fallback for uninitialized theme (black-on-black collision)
            // If they are effectively the same color (e.g. both #000000), force contrast
            if (Math.abs(textColor.r - bgColor.r) < 0.05 && 
                Math.abs(textColor.g - bgColor.g) < 0.05 && 
                Math.abs(textColor.b - bgColor.b) < 0.05) {
                
                let isDark = Kirigami.Theme.colorSet === Kirigami.Theme.Dark || 
                             (Kirigami.Theme.backgroundColor.r + Kirigami.Theme.backgroundColor.g + Kirigami.Theme.backgroundColor.b) < 1.5;
                
                if (isDark) {
                    textColor = "#ffffff";
                    bgColor = "#2a2e32";
                    alternateBgColor = "#31363b";
                    highlightColor = "#3daee9";
                    focusColor = "#3daee9";
                } else {
                    textColor = "#000000";
                    bgColor = "#ffffff";
                    alternateBgColor = "#eff0f1";
                    highlightColor = "#3daee9";
                    focusColor = "#3daee9";
                }
            } else {
                textColor = root.colorToCss(textColor);
                bgColor = root.colorToCss(bgColor);
                alternateBgColor = root.colorToCss(alternateBgColor);
                highlightColor = root.colorToCss(highlightColor);
                focusColor = root.colorToCss(focusColor);
            }

            if (root.articleContent || root.articleTitle) {
                let dateStr = root.publishedAt ? new Date(root.publishedAt).toLocaleString(Qt.locale()) : "";
                let metadata = (root.feedTitle ? root.feedTitle : "") + 
                               (root.feedTitle && root.author ? " • " : "") + 
                               (root.author ? root.author : "") + 
                               ((root.feedTitle || root.author) && dateStr ? " • " : "") + 
                               dateStr;

                let html = `
                    <html>
                    <head>
                        <style>
                            :root {
                                color-scheme: ${Kirigami.Theme.colorSet === Kirigami.Theme.Dark ? 'dark' : 'light'};
                            }
                            body {
                                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
                                padding: 40px;
                                line-height: 1.6;
                                font-size: 16px;
                                color: ${textColor} !important;
                                background-color: ${bgColor} !important;
                                margin: 0 auto;
                                max-width: 800px;
                            }
                            .header {
                                margin-bottom: 40px;
                                border-bottom: 1px solid ${alternateBgColor};
                                padding-bottom: 20px;
                            }
                            .title {
                                font-size: 2.5em;
                                font-weight: 800;
                                line-height: 1.2;
                                margin-bottom: 15px;
                                color: ${textColor};
                            }
                            .meta {
                                font-size: 0.9em;
                                opacity: 0.7;
                                color: ${textColor};
                            }
                            img {
                                max-width: 100% !important;
                                height: auto !important;
                                border-radius: 8px;
                                margin: 20px 0;
                            }
                            pre {
                                background-color: ${alternateBgColor} !important;
                                padding: 15px;
                                border-radius: 8px;
                                overflow-x: auto;
                                color: ${textColor} !important;
                                font-family: 'Fira Code', 'Cascadia Code', monospace;
                                font-size: 14px;
                            }
                            code {
                                font-family: monospace;
                                background-color: ${alternateBgColor};
                                padding: 2px 4px;
                                border-radius: 4px;
                            }
                            a { 
                                color: ${textColor} !important; 
                                font-weight: bold !important; 
                                text-decoration: none !important;
                                border-bottom: 1px solid ${highlightColor};
                            }
                            a:hover { 
                                background-color: ${highlightColor};
                                color: ${bgColor} !important;
                            }
                            blockquote {
                                border-left: 5px solid ${focusColor};
                                margin: 20px 0;
                                padding: 10px 20px;
                                background-color: ${alternateBgColor};
                                font-style: italic;
                                border-radius: 0 8px 8px 0;
                            }
                            /* Native-like Scrollbars */
                            ::-webkit-scrollbar {
                                width: 10px;
                                height: 10px;
                            }
                            ::-webkit-scrollbar-track {
                                background: transparent;
                            }
                            ::-webkit-scrollbar-thumb {
                                background: ${textColor};
                                opacity: 0.2;
                                border-radius: 5px;
                                border: 3px solid transparent;
                                background-clip: content-box;
                            }
                            ::-webkit-scrollbar-thumb:hover {
                                background: ${highlightColor};
                                background-clip: content-box;
                            }
                            /* Content text colors */
                            p, div, span, h1, h2, h3, h4, h5, h6, li {
                                color: ${textColor} !important;
                            }
                        </style>
                    </head>
                    <body>
                        <div class="header">
                            <div class="title">${root.articleTitle}</div>
                            <div class="meta">${metadata}</div>
                        </div>
                        <div class="content">
                            ${root.articleContent}
                        </div>
                    </body>
                    </html>
                `;
                loadHtml(html);
            } else {
                loadHtml("<html><body style='display:flex; justify-content:center; align-items:center; height:90vh; color:#888; font-family:sans-serif; background-color:" + root.colorToCss(bgColor) + ";'>Select an article to read.</body></html>");
            }
        }
        
        Component.onCompleted: {
            // Defer first load to allow Kirigami.Theme to settle
            initialLoadTimer.start();
        }

        Timer {
            id: initialLoadTimer
            interval: 50
            repeat: false
            onTriggered: {
                console.log("Timer triggered. Current Text color:", themedTextColor, "BG color:", themedBgColor);
                root.webViewReady = true;
                articleWebView.updateContent();
            }
        }

        Shortcut {
            sequence: "Space"
            onActivated: articleWebView.runJavaScript("window.scrollBy(0, window.innerHeight);")
        }
    }
}
