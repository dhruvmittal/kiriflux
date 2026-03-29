// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.Dialog {
    id: settingsDialog
    title: i18n("Preferences")
    modal: true
    standardButtons: QQC2.Dialog.Save | QQC2.Dialog.Cancel
    width: 500
    // Grow based on content but cap it at 80% of window height
    height: Math.min(formLayout.implicitHeight + (Kirigami.Units.gridUnit * 5), applicationWindow().height * 0.8)
    anchors.centerIn: parent

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
        }
    }
}
