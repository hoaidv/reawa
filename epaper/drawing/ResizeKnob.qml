import QtQuick

// Visual-only ToolLayer handle. Hit-test lives on leftover canvas (pen + finger).
// @implements [SRS-EP-12]
Item {
    id: root
    property real visualSize: 16
    property real hitSize: 64

    width: hitSize
    height: hitSize
    z: 22

    Rectangle {
        anchors.centerIn: parent
        width: visualSize
        height: visualSize
        color: "white"
        border.color: "black"
        border.width: 2
    }
}
