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

    // @implements [ADR-0019] ToolCanvasLayer — lasso/marquee/AABB + canvasInput
    ToolCanvas {
        id: toolCanvas
        z: 1
        x: 0
        y: 0
        width: drawCanvas.width
        height: drawCanvas.height
        surface: drawCanvas
        session: drawCanvas.session
    }

    // Facts the pointer handlers on ToolCanvas cannot see, and what this app
    // makes of them. Qt exposes no live contact count to QML, and pen proximity
    // reaches no handler at all, so the raw input filter reports both here.
    Connections {
        target: Input

        // @implements [SRS-EP-21] pen near outranks hand touch
        function onPenNearChanged() {
            if (Input.penNear) {
                toolCanvas.cancelHandTouch()
                toolCanvas.cancelInteraction()
            } else {
                toolCanvas.onHoverLeave()
            }
        }

        // @implements [SRS-EP-56] brush hover follows pen near; handlers never see proximity
        function onPenHover(x, y) {
            if (!drawCanvas.session || drawCanvas.session.exclusiveTool !== "erase_brush"
                    || !drawCanvas.session.eraseBrushHover)
                return
            toolCanvas.onHoverMove(x, y)
        }

        // @implements [SRS-EP-24] two contacts outrank a one-finger manip
        function onContactCountChanged() {
            if (Input.contactCount >= 2)
                toolCanvas.onSecondContact()
            else if (Input.contactCount === 0)
                toolCanvas.onContactsCleared()
        }
    }

    // Trailing orientation-top row: DBG | Follow Infini | USB (UI-EP-07).
    // @implements [SRS-EP-50] FollowToggle sibling of ToolChip
    Rectangle {
        id: followToggle
        z: 20
        x: drawCanvas.followToggleRect.x
        y: drawCanvas.followToggleRect.y
        width: drawCanvas.followToggleRect.width
        height: drawCanvas.followToggleRect.height
        color: drawCanvas.followPressed ? "black" : "white"
        border.color: "black"
        border.width: 1

        Image {
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: parent.height * 0.62
            fillMode: Image.PreserveAspectFit
            smooth: false
            source: drawCanvas.followPressed
                    ? "qrc:/icons/icons/icon-epaper-viewport-follow-inv.png"
                    : "qrc:/icons/icons/icon-epaper-viewport-follow.png"
        }

        Canvas {
            id: followHatch
            visible: drawCanvas.followUnavailable
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#000000"
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
                function onFollowChanged() { followHatch.requestPaint() }
            }
        }

        TapHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
            enabled: !drawCanvas.followUnavailable
            onTapped: drawCanvas.tapFollowToggle()
        }
    }

    // Floating tool chip — HT + 3 tools + recognizers + 3 erasers + Undo/Redo.
    Item {
        id: toolChip
        z: 20
        x: drawCanvas.toolChipRect.x
        y: drawCanvas.toolChipRect.y
        width: drawCanvas.toolChipRect.width
        height: drawCanvas.toolChipRect.height

        TapHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
        }

        Row {
            id: chipRow
            spacing: 32
            height: parent.height

            Rectangle {
                id: toolCluster
                width: 12 + 64 * 4
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

                    Rectangle {
                        id: handTouchToggle
                        width: 64
                        height: 64
                        // @implements [SRS-EP-22] btn.hand_touch first primary tile
                        color: toolCanvas.handTouchArmed ? "black" : "white"
                        border.color: "black"
                        border.width: 1
                        Image {
                            anchors.centerIn: parent
                            width: parent.width * 0.62
                            height: parent.height * 0.62
                            fillMode: Image.PreserveAspectFit
                            smooth: false
                            source: toolCanvas.handTouchArmed
                                    ? "qrc:/icons/icons/icon-epaper-hand-inv.png"
                                    : "qrc:/icons/icons/icon-epaper-hand.png"
                        }
                        TapHandler {
                            objectName: "handTap"
                            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
                            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            grabPermissions: PointerHandler.CanTakeOverFromItems
                                             | PointerHandler.ApprovesCancellation
                            onTapped: toolCanvas.toggleHandTouch()
                        }
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

                            TapHandler {
                                acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
                                acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
                                gesturePolicy: TapHandler.ReleaseWithinBounds
                                grabPermissions: PointerHandler.CanTakeOverFromItems
                                                 | PointerHandler.ApprovesCancellation
                                onTapped: drawCanvas.armTool(modelData.id)
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

                            TapHandler {
                                acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
                                acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
                                gesturePolicy: TapHandler.ReleaseWithinBounds
                                grabPermissions: PointerHandler.CanTakeOverFromItems
                                                 | PointerHandler.ApprovesCancellation
                                enabled: !dimmed
                                onTapped: {
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
                id: eraseCluster
                width: 64 * 3
                height: parent.height
                color: "white"
                border.color: "black"
                border.width: 1

                Row {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: [
                            { id: "erase_brush", icon: "icon-epaper-erase-brush" },
                            { id: "erase_area", icon: "icon-epaper-erase-area" },
                            { id: "erase_object", icon: "icon-epaper-erase-object" }
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

                            TapHandler {
                                acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
                                acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
                                gesturePolicy: TapHandler.ReleaseWithinBounds
                                grabPermissions: PointerHandler.CanTakeOverFromItems
                                                 | PointerHandler.ApprovesCancellation
                                onTapped: drawCanvas.armTool(modelData.id)
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

                            TapHandler {
                                acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
                                acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
                                gesturePolicy: TapHandler.ReleaseWithinBounds
                                grabPermissions: PointerHandler.CanTakeOverFromItems
                                                 | PointerHandler.ApprovesCancellation
                                onTapped: {
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

        TapHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
            onTapped: EpaperBridge.restoreXochitl()
        }
    }

    Rectangle {
        id: infiniReconnect
        z: 30
        x: drawCanvas.usbLinkRect.x
        y: drawCanvas.usbLinkRect.y
        width: drawCanvas.usbLinkRect.width
        height: drawCanvas.usbLinkRect.height
        color: "white"
        border.color: "black"
        border.width: 1
        Image {
            anchors.centerIn: parent
            width: parent.width * 0.72
            height: parent.height * 0.72
            fillMode: Image.PreserveAspectFit
            smooth: false
            source: UsbHud.linkState === "connected"
                    ? "qrc:/icons/icons/icon-epaper-usb-connected.png"
                    : UsbHud.linkState === "unplugged"
                      ? "qrc:/icons/icons/icon-epaper-usb-unplugged.png"
                      : "qrc:/icons/icons/icon-epaper-usb-plugged.png"
        }
        TapHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
            onTapped: UsbHud.recoverInfini()
        }
    }

    Rectangle {
        id: debugToggle
        z: 30
        x: drawCanvas.debugToggleRect.x
        y: drawCanvas.debugToggleRect.y
        width: drawCanvas.debugToggleRect.width
        height: drawCanvas.debugToggleRect.height
        color: drawCanvas.debugLogVisible ? "black" : "white"
        border.color: "black"
        border.width: 1
        Text {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 14
            color: drawCanvas.debugLogVisible ? "white" : "black"
            text: "DBG"
        }
        TapHandler {
            id: debugTap
            objectName: "dbgTap"
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
            onTapped: drawCanvas.toggleDebugLog()
        }
    }

    Rectangle {
        id: debugLogBackdrop
        z: 24
        visible: drawCanvas.debugLogVisible
        x: drawCanvas.debugLogRect.x
        y: drawCanvas.debugLogRect.y
        width: drawCanvas.debugLogRect.width
        height: drawCanvas.debugLogRect.height
        color: "white"
        border.color: "black"
        border.width: 1
    }

    Flickable {
        id: debugLogPanel
        z: 25
        visible: drawCanvas.debugLogVisible
        x: drawCanvas.debugLogRect.x
        y: drawCanvas.debugLogRect.y
        width: drawCanvas.debugLogRect.width
        height: drawCanvas.debugLogRect.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: Math.max(height, debugLogText.implicitHeight + 16)
        Text {
            id: debugLogText
            width: debugLogPanel.width - 16
            x: 8
            y: 8
            wrapMode: Text.Wrap
            font.pixelSize: 12
            color: "black"
            text: DebugLog.text
        }
        Connections {
            target: DebugLog
            function onLinesChanged() {
                Qt.callLater(function() {
                    debugLogPanel.contentY = Math.max(0, debugLogPanel.contentHeight - debugLogPanel.height)
                })
            }
        }
    }

    Component.onCompleted: {
        root.canvas = drawCanvas
        EpaperBridge.attachPenModeRegion(drawCanvas)
    }
}
