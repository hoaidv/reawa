import QtQuick
import Epaper 1.0

// Local pen ink on RM2 e-paper (promoted from EXP-0001).
// Input item must stay visible (even if transparent) so width/height are real;
// ink is a pre-created Rectangle pool moved into place (geometry change refreshes).
TabletWindow {
    id: root
    width: Screen.width
    height: Screen.height
    visible: true
    color: "white"
    title: "Epaper"

    property int inkNext: 0
    property string lastSeg: "-"

    function addSeg(x1, y1, x2, y2, w) {
        var it = inkPool.itemAt(inkNext % inkPool.count)
        if (!it)
            return
        var dx = x2 - x1
        var dy = y2 - y1
        var len = Math.max(3, Math.sqrt(dx * dx + dy * dy))
        var thick = Math.max(4, w)
        it.x = x1
        it.y = y1 - thick / 2
        it.width = len
        it.height = thick
        it.rotation = Math.atan2(dy, dx) * 180 / Math.PI
        inkNext++
        root.lastSeg = Math.round(x1) + "," + Math.round(y1)
    }

    TabletCanvas {
        id: drawCanvas
        anchors.fill: parent
        z: 0
        onSegmentDrawn: (x1, y1, x2, y2, w) => addSeg(x1, y1, x2, y2, w)
    }

    Repeater {
        id: inkPool
        model: 2000
        delegate: Rectangle {
            x: 1; y: 1; width: 2; height: 2
            color: "black"
            antialiasing: false
            transformOrigin: Item.Left
            z: 2
        }
    }

    Rectangle {
        id: blink
        x: parent.width - 90; y: 20; width: 60; height: 60; color: "black"; z: 5
        Timer { interval: 800; running: true; repeat: true
                onTriggered: blink.visible = !blink.visible }
    }

    Text {
        z: 10
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 12
        font.pixelSize: 16
        color: "black"
        text: "ink: " + root.inkNext + "  last: " + root.lastSeg
              + "  " + drawCanvas.debugInfo
    }

    Component.onCompleted: root.canvas = drawCanvas
}
