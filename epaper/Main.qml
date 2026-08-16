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

    // @implements [ADR-0019] ToolCanvasLayer — lasso/marquee/AABB, no document blit
    ToolCanvas {
        id: toolCanvas
        z: 1
        x: 0
        y: 0
        width: drawCanvas.width
        height: drawCanvas.height
        canvas: drawCanvas
        visible: false
    }

    // Floating tool chip — 3 exclusive tools + 2 recognizer toggles + Undo/Redo (ADR-0021 / UI-EP-04).
    Item {
        id: toolChip
        z: 20
        x: drawCanvas.toolChipRect.x
        y: drawCanvas.toolChipRect.y
        width: drawCanvas.toolChipRect.width
        height: drawCanvas.toolChipRect.height

        Row {
            id: chipRow
            spacing: 32
            height: parent.height

            Rectangle {
                id: toolCluster
                width: 12 + 64 * 3
                height: parent.height
                color: "white"
                border.color: "black"
                border.width: 1

                Row {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        id: publishStatus
                        width: 12
                        height: parent.height
                        color: "black"
                    }

                    Repeater {
                        model: [
                            { id: "sel_rect", icon: "icon-epaper-sel-rect" },
                            { id: "sel_freeform", icon: "icon-epaper-sel-freeform" },
                            { id: "pen", icon: "icon-epaper-pen" }
                        ]
                        delegate: Rectangle {
                            width: 64
                            height: 64
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
                id: recogCluster
                width: 64 * 2
                height: parent.height
                color: "white"
                border.color: "black"
                border.width: 1

                Row {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: [
                            { id: "ink_box", icon: "icon-epaper-recog-ink-box" },
                            { id: "connector", icon: "icon-epaper-recog-connector" }
                        ]
                        delegate: Rectangle {
                            width: 64
                            height: 64
                            readonly property bool armed: modelData.id === "ink_box"
                                ? drawCanvas.recogInkBoxArmed
                                : drawCanvas.recogConnectorArmed
                            readonly property bool dimmed: drawCanvas.recogTogglesDimmed
                            color: armed ? "black" : "white"
                            border.color: "black"
                            border.width: 1

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

                            Canvas {
                                id: hatch
                                visible: dimmed
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.strokeStyle = armed ? "#ffffff" : "#000000"
                                    ctx.lineWidth = 1
                                    for (var i = -height; i < width + height; i += 4) {
                                        ctx.beginPath()
                                        ctx.moveTo(i, 0)
                                        ctx.lineTo(i + height, height)
                                        ctx.stroke()
                                    }
                                }
                                Connections {
                                    target: drawCanvas
                                    function onToolModeChanged() { hatch.requestPaint() }
                                    function onRecogChanged() { hatch.requestPaint() }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !dimmed
                                onClicked: {
                                    if (modelData.id === "ink_box")
                                        drawCanvas.toggleRecogInkBox()
                                    else
                                        drawCanvas.toggleRecogConnector()
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: historyCluster
                width: 64 * 2
                height: parent.height
                color: "white"
                border.color: "black"
                border.width: 1

                Row {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: [
                            { id: "undo", icon: "icon-epaper-undo" },
                            { id: "redo", icon: "icon-epaper-redo" }
                        ]
                        delegate: Rectangle {
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
                                source: "qrc:/icons/icons/" + modelData.icon + ".png"
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (modelData.id === "undo")
                                        drawCanvas.requestUndo()
                                    else
                                        drawCanvas.requestRedo()
                                }
                            }
                        }
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

    Repeater {
        id: selectHandles
        z: 21
        model: drawCanvas.handleCount
        delegate: Rectangle {
            readonly property var _r: drawCanvas.selectionBoundsRect
            readonly property real _h: drawCanvas.handleSize
            readonly property point _pt: {
                var r = _r
                var n = drawCanvas.handleCount
                var i = index
                if (n === 8) {
                    var p8 = [
                        Qt.point(r.x, r.y),
                        Qt.point(r.x + r.width / 2, r.y),
                        Qt.point(r.x + r.width, r.y),
                        Qt.point(r.x + r.width, r.y + r.height / 2),
                        Qt.point(r.x + r.width, r.y + r.height),
                        Qt.point(r.x + r.width / 2, r.y + r.height),
                        Qt.point(r.x, r.y + r.height),
                        Qt.point(r.x, r.y + r.height / 2)
                    ]
                    return p8[i]
                }
                var p6 = [
                    Qt.point(r.x, r.y),
                    Qt.point(r.x + r.width / 2, r.y),
                    Qt.point(r.x + r.width, r.y),
                    Qt.point(r.x, r.y + r.height),
                    Qt.point(r.x + r.width / 2, r.y + r.height),
                    Qt.point(r.x + r.width, r.y + r.height)
                ]
                return p6[i]
            }
            x: _pt.x - _h / 2
            y: _pt.y - _h / 2
            width: _h
            height: _h
            visible: drawCanvas.handleCount > 0 && _r.width > 0 && _r.height > 0
            color: "white"
            border.color: "black"
            border.width: 2
        }
    }

    Rectangle {
        id: inkScaleChip
        z: 21
        visible: drawCanvas.modeChipVisible
        x: drawCanvas.modeChipRect.x
        y: drawCanvas.modeChipRect.y
        width: Math.max(120, drawCanvas.modeChipRect.width)
        height: Math.max(36, drawCanvas.modeChipRect.height)
        color: "white"
        border.color: "black"
        border.width: 2
        Text {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 14
            color: "black"
            text: drawCanvas.modeChipLabel
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

    // ind.manipulation_unavailable — on the selection overlay, never the debug HUD [SRS-EP-12]
    Rectangle {
        id: manipUnavailable
        z: 21
        visible: drawCanvas.manipulationUnavailable.length > 0
        x: drawCanvas.manipulationUnavailableRect.x
        y: drawCanvas.manipulationUnavailableRect.y
        width: Math.max(220, drawCanvas.manipulationUnavailableRect.width)
        height: Math.max(36, drawCanvas.manipulationUnavailableRect.height)
        color: "white"
        border.color: "black"
        border.width: 2
        Text {
            anchors.fill: parent
            anchors.margins: 6
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 14
            color: "black"
            text: drawCanvas.manipulationUnavailable
        }
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

    // Debug: 64px chip chrome — quit epaper, start xochitl.
    Rectangle {
        id: xochitlSwitch
        z: 30
        x: 8
        y: 8
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
            source: "qrc:/icons/icons/icon-epaper-switch.png"
        }
        // No MouseArea — TabletAppFilter is installed after QML load; a synthetic
        // click here would quit epaper. Pen/finger hit-test is C++.
    }

    Rectangle {
        id: infiniReconnect
        z: 30
        x: root.width - width - 8
        y: 8
        width: 64
        height: 64
        color: "white"
        border.color: "black"
        border.width: 1
        Image {
            x: parent.width * 0.19
            y: parent.height * 0.19
            width: parent.width * 0.62
            height: parent.height * 0.62
            fillMode: Image.PreserveAspectFit
            smooth: false
            source: UsbHud.linkState === "connected"
                    ? "qrc:/icons/icons/icon-epaper-usb-connected.png"
                    : UsbHud.linkState === "unplugged"
                      ? "qrc:/icons/icons/icon-epaper-usb-unplugged.png"
                      : "qrc:/icons/icons/icon-epaper-usb-plugged.png"
        }
        // No MouseArea — ghost click at QML load called recoverInfini() and UDC-unbound a live Mac.
    }
    // @implements [STORY-EP-036]
    Rectangle {
        id: usbHud
        z: 30
        x: 8
        y: root.height - height - 8
        width: root.width - 16
        height: 36
        color: "white"
        border.color: "black"
        border.width: 2
        Text {
            x: 8
            y: 0
            width: parent.width - 16
            height: parent.height
            font.pixelSize: 14
            color: "black"
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            text: "USB  " + UsbHud.status + "  " + UsbHud.debugLine
        }
    }

    Text {
        z: 10
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 8
        anchors.topMargin: {
            var chip = drawCanvas.toolChipRect.y < height / 2
                       ? drawCanvas.toolChipRect.y + drawCanvas.toolChipRect.height + 6
                       : 8
            return Math.max(chip, xochitlSwitch.y + xochitlSwitch.height + 6)
        }
        font.pixelSize: 14
        color: "black"
        text: EpaperBridge.status
              + (EpaperBridge.penModeAttached ? " | pen" : "")
              + (EpaperBridge.monoModeAttached ? " | mono" : "")
              + (EpaperBridge.overlayStrokePen ? " | ovlPen" : "")
              + (drawCanvas.paintsInk ? " | painted" : " | pool " + root.inkNext)
              + " | " + drawCanvas.toolMode
              + (drawCanvas.recogInkBoxArmed ? " ib" : "")
              + (drawCanvas.recogConnectorArmed ? " cn" : "")
              + " | strokes " + drawCanvas.strokeCount
              + "  " + drawCanvas.debugInfo
    }

    Component.onCompleted: {
        root.canvas = drawCanvas
        EpaperBridge.attachPenModeRegion(drawCanvas)
    }
}
