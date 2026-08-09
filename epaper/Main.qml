import QtQuick
import epaper 1.0

// Ink is rasterised by TabletCanvas (QQuickPaintedItem). RM_INK_MODE=pool falls
// back to the Rectangle pool below, which is slower but proven visible on this
// backend. Pen-mode tags the region; direct swapBuffers is opt-in via RM_EP_SWAP.
// @implements [SRS-EP-01]
TabletWindow {
    id: root
    width: Screen.width
    height: Screen.height
    visible: true
    color: "white"
    title: "epaper"

    property int inkNext: 0

    function addSeg(x1, y1, x2, y2, w) {
        if (inkPool.count <= 0)
            return
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
    }

    TabletCanvas {
        id: drawCanvas
        // Bound to the window, not anchored: inside this QQuickWindow subclass
        // `parent` never resolves, which left the item 0x0 and unpainted.
        x: 0
        y: 0
        width: root.width
        height: root.height
        z: 0
        onSegmentDrawn: (x1, y1, x2, y2, w) => addSeg(x1, y1, x2, y2, w)
    }

    Repeater {
        id: inkPool
        model: drawCanvas.paintsInk ? 0 : 2000
        delegate: Rectangle {
            x: 1; y: 1; width: 2; height: 2
            color: "black"
            antialiasing: false
            transformOrigin: Item.Left
            z: 2
        }
    }

    Text {
        z: 10
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 8
        font.pixelSize: 14
        color: "black"
        text: EpaperBridge.status
              + (EpaperBridge.penModeAttached ? " | pen" : "")
              + (drawCanvas.paintsInk ? " | painted" : " | pool " + root.inkNext)
              + " | strokes " + drawCanvas.strokeCount
              + "  " + drawCanvas.debugInfo
    }

    Component.onCompleted: {
        root.canvas = drawCanvas
        EpaperBridge.attachPenModeRegion(drawCanvas)
    }
}
