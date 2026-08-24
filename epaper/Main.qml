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

    // Full-bleed canvas input. Handlers take an exclusive grab, so Qt's own
    // arbitration keeps chrome taps out of the canvas — no hand-written rects.
    // Chrome sits above in z and grabs first (gesturePolicy below).
    // @implements [SRS-EP-04]
    // @implements [SRS-EP-21]
    // @implements [SRS-EP-24]
    Item {
        id: canvasInput
        z: 2
        anchors.fill: drawCanvas

        DragHandler {
            id: penDrag
            objectName: "penDrag"
            target: null
            dragThreshold: 0
            acceptedDevices: PointerDevice.Stylus
            acceptedPointerTypes: PointerDevice.Pen
            // Approve-only: the default CanTakeOverFromHandlersOfDifferentType let
            // this steal the grab a chrome TapHandler had already taken.
            grabPermissions: PointerHandler.ApprovesTakeOverByAnything
            onActiveChanged: {
                if (active)
                    drawCanvas.onPointerStart(centroid.position.x, centroid.position.y,
                                              centroid.pressure, true)
                else
                    drawCanvas.onPointerEnd(centroid.position.x, centroid.position.y, true)
            }
            onCentroidChanged: {
                if (active)
                    drawCanvas.onPointerMove(centroid.position.x, centroid.position.y,
                                             centroid.pressure, true)
            }
        }

        // A DragHandler reports nothing until the point travels, so a stationary
        // finger never reached the canvas at all. Tap-to-select needs its own
        // handler. Chrome refuses takeover and is offered the point first, so
        // this only ever sees taps chrome declined.
        // @implements [SRS-EP-24] one-finger tap selects
        TapHandler {
            id: fingerTap
            objectName: "fingerTap"
            acceptedDevices: PointerDevice.TouchScreen
            acceptedPointerTypes: PointerDevice.Finger
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onTapped: drawCanvas.onFingerTap(point.position.x, point.position.y)
        }

        DragHandler {
            id: fingerDrag
            objectName: "fingerDrag"
            target: null
            dragThreshold: 0
            // Not 1: exceeding the maximum deactivates the handler, and that
            // deactivation raced PinchHandler's takeover — whichever won decided
            // whether a second contact committed a node move or panned.
            maximumPointCount: 2
            acceptedDevices: PointerDevice.TouchScreen
            acceptedPointerTypes: PointerDevice.Finger
            // Must take the grab off fingerTap the moment the finger travels.
            // Safe now that chrome refuses takeover — that refusal, not crippled
            // permissions here, is what protects the buttons.
            grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType
                             | PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesTakeOverByAnything
            onActiveChanged: {
                if (active)
                    drawCanvas.onPointerStart(centroid.position.x, centroid.position.y,
                                              centroid.pressure, false)
                else if (!pinch.active)
                    drawCanvas.onPointerEnd(centroid.position.x, centroid.position.y, false)
            }
            onCentroidChanged: {
                if (active)
                    drawCanvas.onPointerMove(centroid.position.x, centroid.position.y,
                                             centroid.pressure, false)
            }
        }

        // @implements [SRS-EP-24] two-finger pan + uniform scale
        PinchHandler {
            id: pinch
            objectName: "pinch"
            target: null
            acceptedDevices: PointerDevice.TouchScreen
            minimumPointCount: 2
            maximumPointCount: 2
            // Two contacts are already the whole gesture: waiting for Qt's default
            // drag threshold on top of that is what needed the long travel.
            dragThreshold: 0
            // Must outrank the one-finger handler that grabbed the first contact.
            grabPermissions: PointerHandler.CanTakeOverFromAnything
                             | PointerHandler.ApprovesTakeOverByAnything
            onActiveChanged: {
                if (active)
                    drawCanvas.onPinchStart(centroid.position.x, centroid.position.y, activeScale)
                else
                    drawCanvas.onPinchEnd()
            }
            onActiveScaleChanged: {
                if (active)
                    drawCanvas.onPinchUpdate(centroid.position.x, centroid.position.y, activeScale)
            }
            onCentroidChanged: {
                if (active)
                    drawCanvas.onPinchUpdate(centroid.position.x, centroid.position.y, activeScale)
            }
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

    // Floating tool chip — hand-touch toggle + 3 exclusive tools + 2 recognizer toggles + Undo/Redo.
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
                        color: drawCanvas.handTouchArmed ? "black" : "white"
                        border.color: "black"
                        border.width: 1
                        Image {
                            anchors.centerIn: parent
                            width: parent.width * 0.62
                            height: parent.height * 0.62
                            fillMode: Image.PreserveAspectFit
                            smooth: false
                            source: drawCanvas.handTouchArmed
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
                            onTapped: drawCanvas.toggleHandTouch()
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
        TapHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
            onTapped: drawCanvas.encloseSelection()
        }
    }

    Repeater {
        id: selectHandles
        z: 21
        model: drawCanvas.handleCount
        delegate: ResizeKnob {
            readonly property var _r: drawCanvas.selectionBoundsRect
            readonly property real _h: drawCanvas.handleSize
            readonly property real _hit: Math.max(_h, 64)
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
            visualSize: _h
            hitSize: _hit
            x: _pt.x - _hit / 2
            y: _pt.y - _hit / 2
            visible: drawCanvas.handleCount > 0 && _r.width > 0 && _r.height > 0
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
        TapHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
            // Exclusive grab on press, and nothing may take it. The canvas
            // handlers below still get a passive grab on this point; refusing
            // takeover is what keeps them from turning a chrome tap into ink.
            gesturePolicy: TapHandler.ReleaseWithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesCancellation
            onTapped: drawCanvas.tapModeChip()
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
