import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "ui"

Kirigami.ApplicationWindow {
    id: root
    Component.onCompleted: {
        console.log("DEBUG: Triggering forceSync...");
        try {
            syncEngine.forceSync();
        } catch (e) {
            console.log("DEBUG: Caught error during forceSync: " + e);
        }
    }
    
    // Minimal content to satisfy the app
    ArticleListPage {}
}
