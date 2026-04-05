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
        syncEngine.saveSettings();
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
                        text: syncEngine.apiUrl
                        Layout.fillWidth: true
                        onTextEdited: syncEngine.apiUrl = text
                    }
                    
                    SettingLabel { text: i18n("API Key") }
                    QQC2.TextField {
                        id: apiKeyField
                        placeholderText: i18n("Your Miniflux API Key")
                        echoMode: QQC2.TextField.Password
                        text: syncEngine.apiKey
                        Layout.fillWidth: true
                        onTextEdited: syncEngine.apiKey = text
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
                            value: syncEngine.syncInterval
                            Layout.fillWidth: true
                            onValueModified: syncEngine.syncInterval = value
                        }
                        QQC2.Label { text: i18n("minutes") }
                    }

                    SettingLabel { text: i18n("Data Mode") }
                    QQC2.CheckBox {
                        id: cleanSyncCheck
                        text: i18n("Clean Sync (Reset local state on sync, no server updates)")
                        checked: syncEngine.cleanSync
                        Layout.fillWidth: true
                        onToggled: syncEngine.cleanSync = checked
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
                        checked: syncEngine.markAsReadOnOpen
                        Layout.fillWidth: true
                        onToggled: syncEngine.markAsReadOnOpen = checked
                    }

                    SettingLabel { text: i18n("On Scroll") }
                    QQC2.CheckBox {
                        id: markScrollCheck
                        text: i18n("Mark article as read when scrolled past top")
                        checked: syncEngine.markAsReadOnScroll
                        Layout.fillWidth: true
                        onToggled: syncEngine.markAsReadOnScroll = checked
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
                        checked: syncEngine.showToolbarLabels
                        Layout.fillWidth: true
                        onToggled: syncEngine.showToolbarLabels = checked
                    }
                }

                SectionHeader { title: i18n("Layout Widths"); Layout.topMargin: Kirigami.Units.largeSpacing }
                SettingsGrid {
                    SettingLabel { text: i18n("Left Pane Width") }
                    QQC2.SpinBox {
                        id: leftPaneWidthSpin
                        from: 100
                        to: 600
                        value: syncEngine.leftPaneWidth
                        Layout.fillWidth: true
                        onValueModified: syncEngine.leftPaneWidth = value
                    }

                    SettingLabel { text: i18n("Center Pane Width") }
                    QQC2.SpinBox {
                        id: centerPaneWidthSpin
                        from: 200
                        to: 1000
                        value: syncEngine.centerPaneWidth
                        Layout.fillWidth: true
                        onValueModified: syncEngine.centerPaneWidth = value
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
                        fontValue: syncEngine.leftPaneFont
                        onChooseClicked: {
                            currentFontProperty = "leftPaneFont";
                            fontDialog.selectedFont = syncEngine.leftPaneFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article List (Title)")
                        fontValue: syncEngine.articleListTitleFont
                        onChooseClicked: {
                            currentFontProperty = "articleListTitleFont";
                            fontDialog.selectedFont = syncEngine.articleListTitleFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article List (Info)")
                        fontValue: syncEngine.articleListInfoFont
                        onChooseClicked: {
                            currentFontProperty = "articleListInfoFont";
                            fontDialog.selectedFont = syncEngine.articleListInfoFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article View (Title)")
                        fontValue: syncEngine.articleViewTitleFont
                        onChooseClicked: {
                            currentFontProperty = "articleViewTitleFont";
                            fontDialog.selectedFont = syncEngine.articleViewTitleFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article View (Info)")
                        fontValue: syncEngine.articleViewInfoFont
                        onChooseClicked: {
                            currentFontProperty = "articleViewInfoFont";
                            fontDialog.selectedFont = syncEngine.articleViewInfoFont;
                            fontDialog.open();
                        }
                    }
                    FontPicker {
                        labelText: i18n("Article View (Text)")
                        fontValue: syncEngine.articleViewTextFont
                        onChooseClicked: {
                            currentFontProperty = "articleViewTextFont";
                            fontDialog.selectedFont = syncEngine.articleViewTextFont;
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
            if (currentFontProperty === "leftPaneFont") syncEngine.leftPaneFont = selectedFont
            else if (currentFontProperty === "articleListTitleFont") syncEngine.articleListTitleFont = selectedFont
            else if (currentFontProperty === "articleListInfoFont") syncEngine.articleListInfoFont = selectedFont
            else if (currentFontProperty === "articleViewTitleFont") syncEngine.articleViewTitleFont = selectedFont
            else if (currentFontProperty === "articleViewInfoFont") syncEngine.articleViewInfoFont = selectedFont
            else if (currentFontProperty === "articleViewTextFont") syncEngine.articleViewTextFont = selectedFont
        }
    }
}
