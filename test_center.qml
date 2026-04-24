import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 600
    height: 800

    ScrollView {
        anchors.fill: parent
        Text {
            id: txt
            width: parent.width - 20
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            text: "<p>Text before</p><blockquote><div align='center'><table width='400' border='1'><tr><td align='center'><img src='https://images.unsplash.com/photo-1541963463532-d68292c34b19?w=800' width='380'></td></tr></table></div></blockquote><p>Text after</p>"
        }
    }
    Timer { interval: 3000; running: true; onTriggered: Qt.quit() }
}
