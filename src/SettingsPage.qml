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
    height: applicationWindow().height * 0.8
    anchors.centerIn: parent

    property string currentFontProperty: ""

    onAccepted: {
        settings.save();
        syncEngine.forceSync();
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
        default property alias content: innerLayout.data
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
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

    RowLayout {
        anchors.fill: parent
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
                    FontPicker {
                        labelText: i18n("Left Pane (Categories)")
                        fontValue: settings.leftPaneFont
                        onChooseClicked: {
                            currentFontProperty = "leftPaneFont";
                            fontDialog.selectedFont = settings.leftPaneFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article List (Title)")
                        fontValue: settings.articleListTitleFont
                        onChooseClicked: {
                            currentFontProperty = "articleListTitleFont";
                            fontDialog.selectedFont = settings.articleListTitleFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article List (Info)")
                        fontValue: settings.articleListInfoFont
                        onChooseClicked: {
                            currentFontProperty = "articleListInfoFont";
                            fontDialog.selectedFont = settings.articleListInfoFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article View (Title)")
                        fontValue: settings.articleViewTitleFont
                        onChooseClicked: {
                            currentFontProperty = "articleViewTitleFont";
                            fontDialog.selectedFont = settings.articleViewTitleFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article View (Info)")
                        fontValue: settings.articleViewInfoFont
                        onChooseClicked: {
                            currentFontProperty = "articleViewInfoFont";
                            fontDialog.selectedFont = settings.articleViewInfoFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article View (Text)")
                        fontValue: settings.articleViewTextFont
                        onChooseClicked: {
                            currentFontProperty = "articleViewTextFont";
                            fontDialog.selectedFont = settings.articleViewTextFont;
                            fontDialog.open();
                        }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }

    FontDialog {
        id: fontDialog
        title: i18n("Select Font")
        onAccepted: {
            if (currentFontProperty === "leftPaneFont") settings.leftPaneFont = selectedFont
            else if (currentFontProperty === "articleListTitleFont") settings.articleListTitleFont = selectedFont
            else if (currentFontProperty === "articleListInfoFont") settings.articleListInfoFont = selectedFont
            else if (currentFontProperty === "articleViewTitleFont") settings.articleViewTitleFont = selectedFont
            else if (currentFontProperty === "articleViewInfoFont") settings.articleViewInfoFont = selectedFont
            else if (currentFontProperty === "articleViewTextFont") settings.articleViewTextFont = selectedFont
        }
    }
}
