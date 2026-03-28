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
    height: 350
    anchors.centerIn: parent

    onAccepted: {
        syncEngine.saveSettings();
        syncEngine.forceSync();
    }

    Kirigami.FormLayout {
        anchors.fill: parent

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
    }
}
