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
    width: 600
    // Grow based on content but cap it at 80% of window height
    height: Math.min(formLayout.implicitHeight + (Kirigami.Units.gridUnit * 5), applicationWindow().height * 0.9)
    anchors.centerIn: parent

    property string currentFontProperty: ""

    onAccepted: {
        syncEngine.saveSettings();
        syncEngine.forceSync();
    }

    QQC2.ScrollView {
        id: scrollView
        anchors.fill: parent
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

        Kirigami.FormLayout {
            id: formLayout
            width: scrollView.availableWidth

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("Miniflux Configuration")
                Kirigami.FormData.isSection: true
            }

            QQC2.TextField {
                id: apiUrlField
                Kirigami.FormData.label: i18n("API URL")
                placeholderText: "https://miniflux.example.com"
                text: syncEngine.apiUrl
                Layout.fillWidth: true
                onTextEdited: syncEngine.apiUrl = text
            }

            QQC2.TextField {
                id: apiKeyField
                Kirigami.FormData.label: i18n("API Key")
                placeholderText: i18n("Your Miniflux API Key")
                echoMode: QQC2.TextField.Password
                text: syncEngine.apiKey
                Layout.fillWidth: true
                onTextEdited: syncEngine.apiKey = text
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("Sync Options")
                Kirigami.FormData.isSection: true
            }

            QQC2.SpinBox {
                id: intervalSpin
                Kirigami.FormData.label: i18n("Sync Interval (minutes)")
                from: 1
                to: 1440
                value: syncEngine.syncInterval
                onValueModified: syncEngine.syncInterval = value
            }

            QQC2.CheckBox {
                id: cleanSyncCheck
                Kirigami.FormData.label: i18n("Data Mode")
                text: i18n("Clean Sync (Reset local state on sync, no server updates)")
                checked: syncEngine.cleanSync
                onToggled: syncEngine.cleanSync = checked
            }

            QQC2.CheckBox {
                id: showLabelsCheck
                Kirigami.FormData.label: i18n("Appearance")
                text: i18n("Show labels in toolbars")
                checked: syncEngine.showToolbarLabels
                onToggled: syncEngine.showToolbarLabels = checked
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("Reading")
                Kirigami.FormData.isSection: true
            }

            QQC2.CheckBox {
                id: markOpenCheck
                Kirigami.FormData.label: i18n("Automatic Marking")
                text: i18n("Mark article as read when opened")
                checked: syncEngine.markAsReadOnOpen
                onToggled: syncEngine.markAsReadOnOpen = checked
            }

            QQC2.CheckBox {
                id: markScrollCheck
                Kirigami.FormData.label: "" 
                text: i18n("Mark article as read when scrolled past top")
                checked: syncEngine.markAsReadOnScroll
                onToggled: syncEngine.markAsReadOnScroll = checked
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("Fonts")
                Kirigami.FormData.isSection: true
            }

            component FontPicker : RowLayout {
                id: fontPickerRoot
                property string labelText
                property font fontValue
                signal chooseClicked()
                
                Kirigami.FormData.label: labelText
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

            FontPicker {
                labelText: i18n("Left Pane (Categories/Feeds)")
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
