import QtQuick
import epaper 1.0

// Ink is rasterised by TabletCanvas (QQuickPaintedItem).
// ToolChip: floating 64px orientation-top + designer icons (UI-EP-01 human verify).
// @implements [SRS-EP-01]
// @implements [SRS-EP-04]
// @implements [SRS-EP-05]
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
        x: 0
        y: 0
        width: root.width
        height: root.height
        z: 0
        onSegmentDrawn: (x1, y1, x2, y2, w) => addSeg(x1, y1, x2, y2, w)
    }

    // Floating tool chip — hug width, 64px tiles, orientation-top (human verify ≥2× prior 32px)
    Rectangle {
        id: toolChip
        z: 20
        x: drawCanvas.toolChipRect.x
        y: drawCanvas.toolChipRect.y
        width: drawCanvas.toolChipRect.width
        height: drawCanvas.toolChipRect.height
        color: "white"
        border.color: "black"
        border.width: 1

        Row {
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: [
                    { id: "sel_rect", icon: "icon-epaper-sel-rect" },
                    { id: "sel_freeform", icon: "icon-epaper-sel-freeform" },
                    { id: "pen", icon: "icon-epaper-pen" },
                    { id: "ink_box", icon: "icon-epaper-ink-box" }
                ]
                delegate: Rectangle {
                    width: parent.height
                    height: parent.height
                    color: drawCanvas.toolMode === modelData.id ? "black" : "white"
                    border.color: "black"
                    border.width: 1

                    readonly property bool armed: drawCanvas.toolMode === modelData.id

                    Image {
                        anchors.centerIn: parent
                        width: parent.width * 0.62
                        height: parent.height * 0.62
                        fillMode: Image.PreserveAspectFit
                        smooth: false
                        source: armed
                               ? ("qrc:/icons/icons/" + modelData.icon + "-inv.png")
                               : ("qrc:/icons/icons/" + modelData.icon + ".png")
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: drawCanvas.armTool(modelData.id)
                    }
                }
            }
        }
    }

    Rectangle {
        id: encloseCta
        z: 21
        visible: drawCanvas.encloseVisible
        x: drawCanvas.encloseCtaRect.x
        y: drawCanvas.encloseCtaRect.y
        width: 64
        height: 64
        color: "white"
        border.color: "black"
        border.width: 1
        Image {
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: parent.height * 0.62
            fillMode: Image.PreserveAspectFit
            smooth: false
            source: "qrc:/icons/icons/icon-epaper-enclose.png"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: drawCanvas.encloseSelection()
        }
    }

    Text {
        z: 21
        visible: drawCanvas.encloseRefuseReason.length > 0
        x: encloseCta.visible ? encloseCta.x : 24
        y: encloseCta.visible ? encloseCta.y + encloseCta.height + 6 : 80
        font.pixelSize: 16
        color: "black"
        text: drawCanvas.encloseRefuseReason
    }

    Text {
        z: 21
        visible: drawCanvas.manipulationUnavailable.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        y: 72
        font.pixelSize: 16
        color: "black"
        text: drawCanvas.manipulationUnavailable
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
        anchors.topMargin: drawCanvas.toolChipRect.y < height / 2
                           ? drawCanvas.toolChipRect.y + drawCanvas.toolChipRect.height + 6
                           : 8
        font.pixelSize: 14
        color: "black"
        text: EpaperBridge.status
              + (EpaperBridge.penModeAttached ? " | pen" : "")
              + (drawCanvas.paintsInk ? " | painted" : " | pool " + root.inkNext)
              + " | " + drawCanvas.toolMode
              + " | pick " + drawCanvas.pickableCount
              + " | strokes " + drawCanvas.strokeCount
              + "  " + drawCanvas.debugInfo
    }

    Component.onCompleted: {
        root.canvas = drawCanvas
        EpaperBridge.attachPenModeRegion(drawCanvas)
    }
}
