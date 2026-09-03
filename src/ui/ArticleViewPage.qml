// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import QtWebEngine

Kirigami.Page {
    id: root
    objectName: "articleViewPage"

    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // Hint for Kirigami.PageRow (column view)
    Layout.fillWidth: true
    
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

    function colorToCss(c) {
        return "rgba(" + Math.floor(c.r*255) + "," + Math.floor(c.g*255) + "," + Math.floor(c.b*255) + "," + c.a + ")";
    }

    // Hidden TextEdit for clipboard operations
    TextEdit {
        id: clipboardHelper
        visible: false
    }

    function copyToClipboard(text) {
        clipboardHelper.text = text;
        clipboardHelper.selectAll();
        clipboardHelper.copy();
    }

    QQC2.Menu {
        id: contextMenu
        property string linkUrl: ""

        QQC2.MenuItem {
            text: i18n("Copy")
            icon.name: "edit-copy"
            enabled: readerModeLoader.item && readerModeLoader.item.nativeTextItem && readerModeLoader.item.nativeTextItem.selectedText.length > 0
            onTriggered: readerModeLoader.item.nativeTextItem.copy()
        }

        QQC2.MenuItem {
            text: i18n("Copy Link Address")
            icon.name: "edit-copy-url"
            visible: contextMenu.linkUrl !== ""
            onTriggered: root.copyToClipboard(contextMenu.linkUrl)
        }
        
        QQC2.MenuSeparator { visible: contextMenu.linkUrl !== "" }
        
        QQC2.MenuItem {
            text: i18n("Open Link in Browser")
            icon.name: "internet-services"
            visible: contextMenu.linkUrl !== ""
            onTriggered: Qt.openUrlExternally(contextMenu.linkUrl)
        }
    }

    Timer {
        id: renderDebouncer
        interval: 10
        repeat: false
        onTriggered: updateActiveView()
    }

    onArticleContentChanged: if (readerModeLoader.item || webViewLoader.item) renderDebouncer.restart()
    onArticleTitleChanged: if (readerModeLoader.item || webViewLoader.item) renderDebouncer.restart()
    onFeedTitleChanged: if (readerModeLoader.item || webViewLoader.item) renderDebouncer.restart()
    onAuthorChanged: if (readerModeLoader.item || webViewLoader.item) renderDebouncer.restart()
    onPublishedAtChanged: if (readerModeLoader.item || webViewLoader.item) renderDebouncer.restart()

    Connections {
        target: settings
        function onSettingsChanged() {
            updateActiveView();
        }
    }

    function updateActiveView() {
        if (settings && settings.useReaderMode && readerModeLoader.item) {
            readerModeLoader.item.updateContent();
        } else if (settings && !settings.useReaderMode && webViewLoader.item) {
            webViewLoader.item.updateContent();
        }
    }

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: true

    actions: [
        Kirigami.Action {
            text: root.isUnread ? i18n("Mark Read") : i18n("Mark Unread")
            icon.name: root.isUnread ? "mail-mark-read" : "mail-mark-unread"
            onTriggered: {
                if (root.articleId !== null) {
                    articleListModel.markAsReadById(root.articleId, root.isUnread)
                    root.isUnread = !root.isUnread
                }
            }
            displayHint: (settings && settings.showToolbarLabels) ? 0 : 1
        },
        Kirigami.Action {
            text: i18n("Star")
            icon.name: "favorite"
            onTriggered: {
                if (root.articleId !== null) {
                    articleListModel.toggleStarredById(root.articleId)
                }
            }
            displayHint: (settings && settings.showToolbarLabels) ? 0 : 1
        },
        Kirigami.Action {
            text: i18n("Full Content")
            icon.name: "document-import"
            onTriggered: {
                if (root.articleId !== null && syncEngine) {
                    syncEngine.fetchFullContent(root.articleId)
                }
            }
            displayHint: (settings && settings.showToolbarLabels) ? 0 : 1
        },
        Kirigami.Action {
            text: i18n("Open Browser")
            icon.name: "internet-services"
            onTriggered: {
                if (root.articleUrl) {
                    Qt.openUrlExternally(root.articleUrl)
                }
            }
            displayHint: (settings && settings.showToolbarLabels) ? 0 : 1
        }
    ]

    // ── Native Reader Mode (Lightweight, No Chromium) ──────────────────
    Loader {
        id: readerModeLoader
        anchors.fill: parent
        active: settings ? settings.useReaderMode : false
        sourceComponent: Component {
            QQC2.ScrollView {
                id: readerScroll
                QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
                contentWidth: availableWidth
                
                readonly property alias nativeTextItem: nativeText

                property var updateContent: function() {
                    let dateStr = root.publishedAt ? new Date(root.publishedAt).toLocaleString(Qt.locale()) : "";
                    let metadata = (root.feedTitle ? root.feedTitle : "") + 
                                   (root.feedTitle && root.author ? " • " : "") + 
                                   (root.author ? root.author : "") + 
                                   ((root.feedTitle || root.author) && dateStr ? " • " : "") + 
                                   dateStr;
                    
                    nativeTitle.text = root.articleTitle;
                    nativeInfo.text = metadata;
                    
                    // We use availableWidth to account for the ScrollView's vertical scrollbar footprint.
                    // An extra Kirigami.Units.largeSpacing * 12 ensures that inherent blockquote/paragraph indents completely safely center.
                    let maxWidth = Math.max(100, readerScroll.availableWidth - Kirigami.Units.largeSpacing * 12);
                    let processedHtml = root.articleContent;
                    
                    if (processedHtml) {
                        // Simplified Responsive Scaling for QQuickText (RichText)
                        processedHtml = processedHtml.replace(/<img([^>]*)>/gi, function(match, attributes) {
                            let cleanAttrs = attributes.replace(/(width|height)=["'][^"']*["']/gi, "");
                            // We use a simple div wrapper which is better handled by RichText than tables
                            return `<div align="center"><img width="${maxWidth}" ${cleanAttrs}></div>`;
                        });
                    } else {
                        processedHtml = "<p style='color:#888; text-align:center;'>Select an article to read.</p>";
                    }
                    nativeText.text = processedHtml;
                    readerScroll.contentItem.contentY = 0;
                }

                ColumnLayout {
                    width: readerScroll.availableWidth
                    spacing: Kirigami.Units.largeSpacing
                    
                    Item { Layout.fillWidth: true; Layout.preferredHeight: Kirigami.Units.largeSpacing }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.preferredWidth: Kirigami.Units.largeSpacing * 2 }
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.largeSpacing

                            TextEdit {
                                id: nativeTitle
                                readOnly: true
                                selectByMouse: true
                                activeFocusOnTab: false
                                cursorVisible: false
                                selectionColor: Kirigami.Theme.highlightColor
                                selectedTextColor: Kirigami.Theme.highlightedTextColor
                                font.family: (settings && settings.articleViewTitleFont) ? settings.articleViewTitleFont.family : ""
                                font.pointSize: (settings && settings.articleViewTitleFont && settings.articleViewTitleFont.pointSize > 0) ? settings.articleViewTitleFont.pointSize : -1
                                font.weight: (settings && settings.articleViewTitleFont) ? settings.articleViewTitleFont.weight : Font.Normal
                                font.italic: (settings && settings.articleViewTitleFont) ? settings.articleViewTitleFont.italic : false
                                color: root.themedTextColor
                                wrapMode: TextEdit.WordWrap
                                Layout.fillWidth: true

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton
                                    onClicked: {
                                        contextMenu.linkUrl = "";
                                        contextMenu.popup();
                                    }
                                }
                            }
                            
                            TextEdit {
                                id: nativeInfo
                                readOnly: true
                                selectByMouse: true
                                activeFocusOnTab: false
                                cursorVisible: false
                                selectionColor: Kirigami.Theme.highlightColor
                                selectedTextColor: Kirigami.Theme.highlightedTextColor
                                font.family: (settings && settings.articleViewInfoFont) ? settings.articleViewInfoFont.family : ""
                                font.pointSize: (settings && settings.articleViewInfoFont && settings.articleViewInfoFont.pointSize > 0) ? settings.articleViewInfoFont.pointSize : -1
                                font.weight: (settings && settings.articleViewInfoFont) ? settings.articleViewInfoFont.weight : Font.Normal
                                font.italic: (settings && settings.articleViewInfoFont) ? settings.articleViewInfoFont.italic : false
                                color: root.themedTextColor
                                opacity: 0.7
                                wrapMode: TextEdit.WordWrap
                                Layout.fillWidth: true

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton
                                    onClicked: {
                                        contextMenu.linkUrl = "";
                                        contextMenu.popup();
                                    }
                                }
                            }
                            
                            Kirigami.Separator { Layout.fillWidth: true }
                            
                            TextEdit {
                                id: nativeText
                                textFormat: TextEdit.RichText
                                readOnly: true
                                selectByMouse: true
                                activeFocusOnTab: false
                                cursorVisible: false
                                selectionColor: Kirigami.Theme.highlightColor
                                selectedTextColor: Kirigami.Theme.highlightedTextColor
                                baseUrl: root.articleUrl
                                font.family: (settings && settings.articleViewTextFont) ? settings.articleViewTextFont.family : ""
                                font.pointSize: (settings && settings.articleViewTextFont && settings.articleViewTextFont.pointSize > 0) ? settings.articleViewTextFont.pointSize : -1
                                font.weight: (settings && settings.articleViewTextFont) ? settings.articleViewTextFont.weight : Font.Normal
                                font.italic: (settings && settings.articleViewTextFont) ? settings.articleViewTextFont.italic : false
                                color: root.themedTextColor
                                wrapMode: TextEdit.WordWrap
                                Layout.fillWidth: true
                                onLinkActivated: (link) => Qt.openUrlExternally(link)
                                
                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton
                                    onClicked: (mouse) => {
                                        let link = nativeText.linkAt(mouse.x, mouse.y);
                                        contextMenu.linkUrl = link;
                                        contextMenu.popup();
                                    }
                                }
                                
                                HoverHandler {
                                    cursorShape: (nativeText.linkAt(point.position.x, point.position.y) !== "") ? Qt.PointingHandCursor : Qt.IBeamCursor
                                }
                            }
                            
                            Item { Layout.fillWidth: true; Layout.preferredHeight: Kirigami.Units.largeSpacing * 4 }
                        }
                        
                        Item { Layout.preferredWidth: Kirigami.Units.largeSpacing * 2 }
                    }
                }
                
                Component.onCompleted: updateContent()

                Shortcut {
                    sequence: "Space"
                    onActivated: {
                        let flick = readerScroll.contentItem
                        let maxScroll = Math.max(0, flick.contentHeight - flick.height)
                        flick.contentY = Math.min(flick.contentY + flick.height, maxScroll)
                    }
                }

                Shortcut {
                    sequence: "Shift+Space"
                    onActivated: {
                        let flick = readerScroll.contentItem
                        flick.contentY = Math.max(flick.contentY - flick.height, 0)
                    }
                }
            }
        }
    }

    // ── Chromium WebEngine Mode (Heavy, Complex layout support) ──────────
    Loader {
        id: webViewLoader
        anchors.fill: parent
        active: settings ? !settings.useReaderMode : false
        sourceComponent: Component {
            WebEngineView {
                id: articleWebView
                backgroundColor: root.themedBgColor
                
                settings.pluginsEnabled: false
                settings.javascriptEnabled: true
                
                property bool webViewReady: false

                onNavigationRequested: function(request) {
                    if (request.navigationType === WebEngineNavigationRequest.LinkClickedNavigation) {
                        Qt.openUrlExternally(request.url);
                        request.action = WebEngineNavigationRequest.IgnoreRequest;
                    } else {
                        request.action = WebEngineNavigationRequest.AcceptRequest;
                    }
                }

                onLoadingChanged: (loadingInfo) => {
                    if (loadingInfo.status === WebEngineView.LoadSucceededStatus) {
                        if (!webViewReady) {
                            webViewReady = true;
                            updateTheme();
                            updateContent();
                        }
                        runJavaScript("window.scrollTo(0, 0);");
                    }
                }

                property var loadTemplate: function() {
                    let html = `
                        <html>
                        <head>
                            <style>
                                :root { 
                                    --color-scheme: light;
                                    --text-color: #000;
                                    --bg-color: #fff;
                                    --alt-bg-color: #eee;
                                    --highlight-color: #3daee9;
                                    --focus-color: #3daee9;
                                }
                                color-scheme: var(--color-scheme);
                                body {
                                    font-family: "${(settings && settings.articleViewTextFont) ? settings.articleViewTextFont.family : 'serif'}", serif;
                                    font-size: ${(settings && settings.articleViewTextFont && settings.articleViewTextFont.pointSize > 0) ? settings.articleViewTextFont.pointSize + "pt" : '12pt'};
                                    font-weight: ${(settings && settings.articleViewTextFont) ? settings.articleViewTextFont.weight : 'normal'};
                                    font-style: ${(settings && settings.articleViewTextFont) ? (settings.articleViewTextFont.italic ? 'italic' : 'normal') : 'normal'};
                                    padding: 40px; line-height: 1.6;
                                    color: var(--text-color) !important; background-color: var(--bg-color) !important;
                                    margin: 0 auto; max-width: 800px;
                                }
                                .header { margin-bottom: 40px; border-bottom: 1px solid var(--alt-bg-color); padding-bottom: 20px; }
                                .title {
                                    font-family: "${(settings && settings.articleViewTitleFont) ? settings.articleViewTitleFont.family : 'sans-serif'}", sans-serif;
                                    font-size: ${(settings && settings.articleViewTitleFont && settings.articleViewTitleFont.pointSize > 0) ? settings.articleViewTitleFont.pointSize + "pt" : '18pt'};
                                    font-weight: ${(settings && settings.articleViewTitleFont) ? settings.articleViewTitleFont.weight : 'bold'};
                                    font-style: ${(settings && settings.articleViewTitleFont) ? (settings.articleViewTitleFont.italic ? 'italic' : 'normal') : 'normal'};
                                    line-height: 1.2; margin-bottom: 15px; color: var(--text-color);
                                }
                                .meta {
                                    font-family: "${(settings && settings.articleViewInfoFont) ? settings.articleViewInfoFont.family : 'sans-serif'}", sans-serif;
                                    font-size: ${(settings && settings.articleViewInfoFont && settings.articleViewInfoFont.pointSize > 0) ? settings.articleViewInfoFont.pointSize + "pt" : '10pt'};
                                    font-weight: ${(settings && settings.articleViewInfoFont) ? settings.articleViewInfoFont.weight : 'normal'};
                                    font-style: ${(settings && settings.articleViewInfoFont) ? (settings.articleViewInfoFont.italic ? 'italic' : 'normal') : 'normal'};
                                    opacity: 0.7; color: var(--text-color);
                                }
                                img { 
                                    max-width: 100% !important; 
                                    height: auto !important; 
                                    border-radius: 8px; 
                                    margin: 20px 0; 
                                    background-color: var(--alt-bg-color);
                                    transition: opacity 0.3s ease-in;
                                }
                                pre {
                                    background-color: var(--alt-bg-color) !important; padding: 15px; border-radius: 8px;
                                    overflow-x: auto; color: var(--text-color) !important;
                                    font-family: 'Fira Code', 'Cascadia Code', monospace; font-size: 14px;
                                }
                                code { font-family: monospace; background-color: var(--alt-bg-color); padding: 2px 4px; border-radius: 4px; }
                                a { color: var(--text-color) !important; font-weight: bold !important; text-decoration: none !important; border-bottom: 1px solid var(--highlight-color); }
                                a:hover { background-color: var(--highlight-color); color: var(--bg-color) !important; }
                                blockquote {
                                    border-left: 5px solid var(--focus-color); margin: 20px 0; padding: 10px 20px;
                                    background-color: var(--alt-bg-color); font-style: italic; border-radius: 0 8px 8px 0;
                                }
                                ::-webkit-scrollbar { width: 10px; height: 10px; }
                                ::-webkit-scrollbar-track { background: transparent; }
                                ::-webkit-scrollbar-thumb {
                                    background: var(--text-color); opacity: 0.2; border-radius: 5px;
                                    border: 3px solid transparent; background-clip: content-box;
                                }
                                ::-webkit-scrollbar-thumb:hover { background: var(--highlight-color); background-clip: content-box; }
                                p, div, span, h1, h2, h3, h4, h5, h6, li { color: var(--text-color) !important; }
                            </style>
                        </head>
                        <body>
                            <div class="header">
                                <div class="title" id="article-title"></div>
                                <div class="meta" id="article-meta"></div>
                            </div>
                            <div class="content" id="article-content">
                                <div style="display:flex; justify-content:center; align-items:center; height:50vh; color:#888;">Loading...</div>
                            </div>
                        </body>
                        </html>
                    `;
                    loadHtml(html);
                }

                property var updateTheme: function() {
                    let textColor = root.themedTextColor;
                    let bgColor = root.themedBgColor;
                    let alternateBgColor = root.themedAlternateBgColor;
                    let highlightColor = root.themedHighlightColor;
                    let focusColor = root.themedFocusColor;

                    let isDark = (Application.styleHints.colorScheme === Qt.Dark) || 
                                 (Kirigami.Theme.backgroundColor.r + Kirigami.Theme.backgroundColor.g + Kirigami.Theme.backgroundColor.b) < 1.5;

                    if (Math.abs(textColor.r - bgColor.r) < 0.05 && 
                        Math.abs(textColor.g - bgColor.g) < 0.05 && 
                        Math.abs(textColor.b - bgColor.b) < 0.05) {
                        
                        if (isDark) {
                            textColor = "#ffffff"; bgColor = "#2a2e32"; alternateBgColor = "#31363b";
                            highlightColor = "#3daee9"; focusColor = "#3daee9";
                        } else {
                            textColor = "#000000"; bgColor = "#ffffff"; alternateBgColor = "#eff0f1";
                            highlightColor = "#3daee9"; focusColor = "#3daee9";
                        }
                    } else {
                        textColor = root.colorToCss(textColor); bgColor = root.colorToCss(bgColor);
                        alternateBgColor = root.colorToCss(alternateBgColor);
                        highlightColor = root.colorToCss(highlightColor);
                        focusColor = root.colorToCss(focusColor);
                    }

                    if (!webViewReady) return;

                    let colorScheme = isDark ? 'dark' : 'light';

                    runJavaScript(`
                        document.documentElement.style.setProperty('--color-scheme', '${colorScheme}');
                        document.documentElement.style.setProperty('--text-color', '${textColor}');
                        document.documentElement.style.setProperty('--bg-color', '${bgColor}');
                        document.documentElement.style.setProperty('--alt-bg-color', '${alternateBgColor}');
                        document.documentElement.style.setProperty('--highlight-color', '${highlightColor}');
                        document.documentElement.style.setProperty('--focus-color', '${focusColor}');
                    `);
                }

                Connections {
                    target: Kirigami.Theme
                    function onBackgroundColorChanged() {
                        if (articleWebView.webViewReady) {
                            articleWebView.updateTheme();
                        }
                    }
                }

                Connections {
                    target: Application.styleHints
                    function onColorSchemeChanged() {
                        if (articleWebView.webViewReady) {
                            articleWebView.updateTheme();
                        }
                    }
                }

                property var updateContent: function() {
                    if (!webViewReady) return;

                    if (root.articleContent || root.articleTitle) {
                        let dateStr = root.publishedAt ? new Date(root.publishedAt).toLocaleString(Qt.locale()) : "";
                        let metadata = (root.feedTitle ? root.feedTitle : "") + 
                                       (root.feedTitle && root.author ? " • " : "") + 
                                       (root.author ? root.author : "") + 
                                       ((root.feedTitle || root.author) && dateStr ? " • " : "") + 
                                       dateStr;

                        let titleHtml = root.articleTitle || "";
                        let contentHtml = root.articleContent || "<p>No content available.</p>";

                        runJavaScript(`
                            document.getElementById('article-title').innerHTML = ${JSON.stringify(titleHtml)};
                            document.getElementById('article-meta').textContent = ${JSON.stringify(metadata)};
                            document.getElementById('article-content').innerHTML = ${JSON.stringify(contentHtml)};
                            window.scrollTo(0, 0);
                        `);
                    } else {
                        let emptyHtml = "<div style='display:flex; justify-content:center; align-items:center; height:50vh; color:#888;'>Select an article to read.</div>";
                        runJavaScript(`
                            document.getElementById('article-title').innerHTML = "";
                            document.getElementById('article-meta').textContent = "";
                            document.getElementById('article-content').innerHTML = ${JSON.stringify(emptyHtml)};
                            window.scrollTo(0, 0);
                        `);
                    }
                }
                
                Component.onCompleted: {
                    initialLoadTimer.start();
                }

                Timer {
                    id: initialLoadTimer
                    interval: 50
                    repeat: false
                    onTriggered: {
                        articleWebView.loadTemplate();
                    }
                }

                Shortcut {
                    sequence: "Space"
                    onActivated: articleWebView.runJavaScript("window.scrollBy(0, window.innerHeight);")
                }
            }
        }
    }

    // Global page shortcuts context
    Shortcut {
        sequence: "s"
        onActivated: {
            if (root.articleId !== null) {
                articleListModel.toggleStarredById(root.articleId)
            }
        }
    }

    Shortcut {
        sequence: "d"
        onActivated: {
            if (root.articleId !== null && syncEngine) {
                syncEngine.fetchFullContent(root.articleId)
            }
        }
    }

    Shortcut {
        sequence: "m"
        onActivated: {
            if (root.articleId !== null) {
                articleListModel.markAsReadById(root.articleId, root.isUnread)
                root.isUnread = !root.isUnread
            }
        }
    }

    Shortcut {
        sequence: "f"
        onActivated: {
            if (root.articleUrl) {
                Qt.openUrlExternally(root.articleUrl)
            }
        }
    }
}
