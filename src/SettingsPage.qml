// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

QQC2.Dialog {
    id: settingsDialog
    title: i18n("Preferences")
    modal: true
    standardButtons: QQC2.Dialog.Save | QQC2.Dialog.Cancel
    width: 750
    height: applicationWindow() ? applicationWindow().height * 0.8 : 600
    anchors.centerIn: parent

    property string currentFontProperty: ""

    onAccepted: {
        settings.save();
        syncEngine.forceSync();
    }

    onRejected: {
        settings.load();
    }

    component FontPicker : RowLayout {
        id: fontPickerRoot
        property string labelText
        property font fontValue
        signal chooseClicked()
        
        SettingLabel { text: labelText }
        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                text: fontPickerRoot.fontValue.family + " (" + (fontPickerRoot.fontValue.pixelSize > 0 ? fontPickerRoot.fontValue.pixelSize + "px" : fontPickerRoot.fontValue.pointSize + "pt") + ")"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            QQC2.Button {
                icon.name: "preferences-desktop-font"
                text: i18n("Choose...")
                onClicked: fontPickerRoot.chooseClicked()
            }
        }
    }

    component SettingsPageContent : QQC2.ScrollView {
        id: pageScroll
        default property alias content: innerLayout.data
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
        contentWidth: availableWidth
        
        ColumnLayout {
            width: pageScroll.availableWidth
            spacing: 0
            
            Item { Layout.fillWidth: true; Layout.preferredHeight: Kirigami.Units.largeSpacing }
            
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.preferredWidth: Kirigami.Units.largeSpacing * 2 }
                
                ColumnLayout {
                    id: innerLayout
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing
                }
                
                Item { Layout.preferredWidth: Kirigami.Units.largeSpacing * 2 }
            }
            
            Item { Layout.fillWidth: true; Layout.preferredHeight: Kirigami.Units.largeSpacing * 2 }
        }
    }

    component SectionHeader : ColumnLayout {
        property string title
        spacing: Kirigami.Units.smallSpacing
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.smallSpacing
        QQC2.Label {
            text: title
            font.weight: Font.DemiBold
            color: Kirigami.Theme.highlightColor
        }
        Kirigami.Separator {
            Layout.fillWidth: true
        }
    }

    component SettingsGrid : GridLayout {
        columns: 2
        columnSpacing: Kirigami.Units.largeSpacing * 2
        rowSpacing: Kirigami.Units.largeSpacing
        Layout.fillWidth: true
    }

    component SettingLabel : QQC2.Label {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }

    Loader {
        anchors.fill: parent
        active: settingsDialog.opened
        sourceComponent: RowLayout {
            spacing: 0

            // Sidebar
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                color: Kirigami.Theme.alternateBackgroundColor

                ListView {
                    id: categoryList
                    anchors.fill: parent
                    model: ListModel {
                        ListElement { name: "Account"; iconSource: "user-identity" }
                        ListElement { name: "Sync"; iconSource: "view-refresh" }
                        ListElement { name: "Reading"; iconSource: "view-list-details" }
                        ListElement { name: "Appearance"; iconSource: "preferences-desktop-theme" }
                        ListElement { name: "Fonts"; iconSource: "preferences-desktop-font" }
                    }
                    currentIndex: 0
                    clip: true
                    
                    delegate: QQC2.ItemDelegate {
                        width: ListView.view.width
                        height: Kirigami.Units.gridUnit * 5
                        highlighted: ListView.isCurrentItem
                        onClicked: categoryList.currentIndex = index

                        contentItem: ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Icon {
                                source: model.iconSource
                                implicitWidth: Kirigami.Units.iconSizes.medium
                                implicitHeight: Kirigami.Units.iconSizes.medium
                                Layout.alignment: Qt.AlignHCenter
                            }
                            
                            QQC2.Label {
                                text: i18n(model.name)
                                Layout.alignment: Qt.AlignHCenter
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
            }

            // Content Area
            StackLayout {
                id: stackLayout
                currentIndex: categoryList.currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Page 0: Account
                SettingsPageContent {
                    SectionHeader { title: i18n("Miniflux Configuration") }
                    SettingsGrid {
                        SettingLabel { text: i18n("API URL") }
                        QQC2.TextField {
                            id: apiUrlField
                            placeholderText: "https://miniflux.example.com"
                            text: settings.apiUrl
                            Layout.fillWidth: true
                            onTextEdited: settings.apiUrl = text
                        }
                        
                        SettingLabel { text: i18n("API Key") }
                        QQC2.TextField {
                            id: apiKeyField
                            placeholderText: i18n("Your Miniflux API Key")
                            echoMode: QQC2.TextField.Password
                            text: settings.apiKey
                            Layout.fillWidth: true
                            onTextEdited: settings.apiKey = text
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Page 1: Sync
                SettingsPageContent {
                    SectionHeader { title: i18n("Sync Options") }
                    SettingsGrid {
                        SettingLabel { text: i18n("Sync Interval") }
                        RowLayout {
                            Layout.fillWidth: true
                            QQC2.SpinBox {
                                id: intervalSpin
                                from: 1
                                to: 1440
                                value: settings.syncInterval
                                Layout.fillWidth: true
                                onValueModified: settings.syncInterval = value
                            }
                            QQC2.Label { text: i18n("minutes") }
                        }

                        SettingLabel { text: i18n("Data Mode") }
                        QQC2.CheckBox {
                            id: cleanSyncCheck
                            text: i18n("Clean Sync (Reset local state on sync, no server updates)")
                            checked: settings.cleanSync
                            Layout.fillWidth: true
                            onToggled: settings.cleanSync = checked
                        }
                    }

                    SectionHeader { title: i18n("Performance & Cache"); Layout.topMargin: Kirigami.Units.largeSpacing }
                    SettingsGrid {
                        SettingLabel { text: i18n("Lazy Loading") }
                        QQC2.CheckBox {
                            id: lazyLoadingCheck
                            text: i18n("Load articles dynamically while scrolling")
                            checked: settings.lazyLoadingEnabled
                            Layout.fillWidth: true
                            onToggled: settings.lazyLoadingEnabled = checked
                        }

                        SettingLabel { text: i18n("Batch Size") }
                        RowLayout {
                            Layout.fillWidth: true
                            QQC2.SpinBox {
                                id: batchSizeSpin
                                from: 10
                                to: 500
                                stepSize: 10
                                value: settings.lazyLoadingBatchSize
                                Layout.fillWidth: true
                                onValueModified: settings.lazyLoadingBatchSize = value
                            }
                            QQC2.Label { text: i18n("articles") }
                        }

                        SettingLabel { text: i18n("Pre-fetching") }
                        QQC2.CheckBox {
                            id: prefetchCheck
                            text: i18n("Background Image Pre-fetching")
                            checked: settings.preFetchImages
                            Layout.fillWidth: true
                            onToggled: settings.preFetchImages = checked
                        }

                        SettingLabel { text: i18n("Cache Size") }
                        RowLayout {
                            Layout.fillWidth: true
                            QQC2.SpinBox {
                                id: cacheSizeSpin
                                from: 32
                                to: 4096
                                stepSize: 32
                                value: settings.cacheSizeMB
                                Layout.fillWidth: true
                                onValueModified: settings.cacheSizeMB = value
                            }
                            QQC2.Label { text: i18n("MB") }
                        }

                        SettingLabel { text: i18n("Database Pruning") }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing
                            QQC2.CheckBox {
                                text: i18n("Automatically purge old read articles")
                                checked: settings.autoPruneEnabled
                                onToggled: settings.autoPruneEnabled = checked
                            }
                            RowLayout {
                                enabled: settings.autoPruneEnabled
                                QQC2.Label { text: i18n("Keep read articles for:") }
                                QQC2.SpinBox {
                                    from: 1
                                    to: 365
                                    value: settings.pruneAfterDays
                                    onValueModified: settings.pruneAfterDays = value
                                }
                                QQC2.Label { text: i18n("days") }
                            }
                        }

                        SettingLabel { text: i18n("Maintenance") }
                        RowLayout {
                            spacing: Kirigami.Units.largeSpacing
                            QQC2.Button {
                                text: i18n("Clear Image Cache")
                                icon.name: "edit-clear-history"
                                onClicked: clearCacheDialog.open()
                            }
                            QQC2.Button {
                                text: i18n("Optimize Database Now")
                                icon.name: "database-index"
                                onClicked: {
                                    databaseManager.optimizeDatabase();
                                    applicationWindow().showPassiveNotification(i18n("Database optimized and vacuumed successfully."))
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Page 2: Reading
                SettingsPageContent {
                    SectionHeader { title: i18n("Automatic Marking") }
                    SettingsGrid {
                        SettingLabel { text: i18n("On Open") }
                        QQC2.CheckBox {
                            id: markOpenCheck
                            text: i18n("Mark article as read when opened")
                            checked: settings.markAsReadOnOpen
                            Layout.fillWidth: true
                            onToggled: settings.markAsReadOnOpen = checked
                        }

                        SettingLabel { text: i18n("On Scroll") }
                        QQC2.CheckBox {
                            id: markScrollCheck
                            text: i18n("Mark article as read when scrolled past top")
                            checked: settings.markAsReadOnScroll
                            Layout.fillWidth: true
                            onToggled: settings.markAsReadOnScroll = checked
                        }
                    }

                    SectionHeader { title: i18n("Display Mode"); Layout.topMargin: Kirigami.Units.largeSpacing }
                    SettingsGrid {
                        SettingLabel { text: i18n("Reader Mode") }
                        QQC2.CheckBox {
                            id: readerModeCheck
                            text: i18n("Use Native Reader Mode (No Chromium)")
                            checked: settings.useReaderMode
                            Layout.fillWidth: true
                            onToggled: settings.useReaderMode = checked
                        }

                        SettingLabel { text: i18n("Reading Pane") }
                        QQC2.CheckBox {
                            id: closeReadingPaneCheck
                            text: i18n("Close reading pane when switching filters or feeds")
                            checked: settings.closeReadingPaneOnReset
                            Layout.fillWidth: true
                            onToggled: settings.closeReadingPaneOnReset = checked
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Page 3: Appearance
                SettingsPageContent {
                    SectionHeader { title: i18n("General Appearance") }
                    SettingsGrid {
                        SettingLabel { text: i18n("Toolbars") }
                        QQC2.CheckBox {
                            id: showLabelsCheck
                            text: i18n("Show labels in toolbars")
                            checked: settings.showToolbarLabels
                            Layout.fillWidth: true
                            onToggled: settings.showToolbarLabels = checked
                        }
                    }

                    SectionHeader { title: i18n("Layout Widths"); Layout.topMargin: Kirigami.Units.largeSpacing }
                    SettingsGrid {
                        SettingLabel { text: i18n("Left Pane Width") }
                        QQC2.SpinBox {
                            id: leftPaneWidthSpin
                            from: 100
                            to: 600
                            value: settings.leftPaneWidth
                            Layout.fillWidth: true
                            onValueModified: settings.leftPaneWidth = value
                        }

                        SettingLabel { text: i18n("Center Pane Width") }
                        QQC2.SpinBox {
                            id: centerPaneWidthSpin
                            from: 200
                            to: 1000
                            value: settings.centerPaneWidth
                            Layout.fillWidth: true
                            onValueModified: settings.centerPaneWidth = value
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Page 4: Fonts
                SettingsPageContent {
                    SectionHeader { title: i18n("Application Fonts") }
                    SettingsGrid {
                        Repeater {
                            model: [
                                { label: i18n("Left Pane (Categories)"), key: "leftPaneFont" },
                                { label: i18n("Article List (Title)"), key: "articleListTitleFont" },
                                { label: i18n("Article List (Info)"), key: "articleListInfoFont" },
                                { label: i18n("Article View (Title)"), key: "articleViewTitleFont" },
                                { label: i18n("Article View (Info)"), key: "articleViewInfoFont" },
                                { label: i18n("Article View (Text)"), key: "articleViewTextFont" }
                            ]
                            delegate: FontPicker {
                                required property var modelData
                                labelText: modelData.label
                                fontValue: settings[modelData.key]
                                onChooseClicked: {
                                    settingsDialog.currentFontProperty = modelData.key;
                                    fontDialog.selectedFont = settings[modelData.key];
                                    fontDialog.open();
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    FontDialog {
        id: fontDialog
        title: i18n("Select Font")
        onAccepted: {
            if (currentFontProperty) {
                settings[currentFontProperty] = selectedFont;
            }
        }
    }

    MessageDialog {
        id: clearCacheDialog
        title: i18n("Clear Cache")
        text: i18n("Are you sure you want to clear the image cache? This will redownload images when articles are viewed.")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: networkManager.clearCache()
    }
}
