import QtQuick
import epaper 1.0

// Peer of TabletCanvas: always-on hit target + visibility-gated ToolCanvasItem overlay
// (Mono mode only while selection chrome is shown — ADR-0019).
Item {
    id: root

    property alias surface: tool.surface
    property alias session: tool.session

    property alias handTouchArmed: tool.handTouchArmed
    property alias selectionBar: tool.selectionBar

    function toggleHandTouch() { tool.toggleHandTouch() }
    function cancelHandTouch() { tool.cancelHandTouch() }
    function cancelInteraction() { tool.cancelInteraction() }
    function onHoverLeave() { tool.onHoverLeave() }
    function onSecondContact() { tool.onSecondContact() }
    function onContactsCleared() { tool.onContactsCleared() }

    ToolCanvasItem {
        id: tool
        anchors.fill: parent
        // C++ syncToolCanvasPresence toggles visible for Mono attach scope.
    }

    // Full-bleed canvas input — always active regardless of overlay visibility.
    // @implements [SRS-EP-04]
    // @implements [SRS-EP-21]
    // @implements [SRS-EP-24]
    Item {
        id: canvasInput
        anchors.fill: parent
        z: 1

        DragHandler {
            id: penDrag
            objectName: "penDrag"
            target: null
            dragThreshold: 0
            acceptedDevices: PointerDevice.Stylus
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Eraser
            grabPermissions: PointerHandler.ApprovesTakeOverByAnything
            property bool eraserNib: false
            onActiveChanged: {
                if (active)
                    eraserNib = centroid.pointerType === PointerDevice.Eraser
                if (active)
                    tool.onPointerStart(centroid.position.x, centroid.position.y,
                                        centroid.pressure, true, eraserNib)
                else
                    tool.onPointerEnd(centroid.position.x, centroid.position.y, true, eraserNib)
            }
            onCentroidChanged: {
                if (active)
                    tool.onPointerMove(centroid.position.x, centroid.position.y,
                                       centroid.pressure, true, eraserNib)
            }
        }

        HoverHandler {
            id: penHover
            acceptedDevices: PointerDevice.Stylus
            acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Eraser
            enabled: session && session.exclusiveTool === "erase_brush"
                     && session.eraseBrushHover
            onPointChanged: tool.onHoverMove(point.position.x, point.position.y)
            onHoveredChanged: {
                if (!hovered)
                    tool.onHoverLeave()
            }
        }

        TapHandler {
            id: fingerTap
            objectName: "fingerTap"
            acceptedDevices: PointerDevice.TouchScreen
            acceptedPointerTypes: PointerDevice.Finger
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onTapped: tool.onFingerTap(point.position.x, point.position.y)
        }

        DragHandler {
            id: fingerDrag
            objectName: "fingerDrag"
            target: null
            dragThreshold: 0
            maximumPointCount: 2
            acceptedDevices: PointerDevice.TouchScreen
            acceptedPointerTypes: PointerDevice.Finger
            grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType
                             | PointerHandler.CanTakeOverFromItems
                             | PointerHandler.ApprovesTakeOverByAnything
            onActiveChanged: {
                if (active)
                    tool.onPointerStart(centroid.position.x, centroid.position.y,
                                        centroid.pressure, false)
                else if (!pinch.active)
                    tool.onPointerEnd(centroid.position.x, centroid.position.y, false)
            }
            onCentroidChanged: {
                if (active)
                    tool.onPointerMove(centroid.position.x, centroid.position.y,
                                       centroid.pressure, false)
            }
        }

        PinchHandler {
            id: pinch
            objectName: "pinch"
            target: null
            acceptedDevices: PointerDevice.TouchScreen
            minimumPointCount: 2
            maximumPointCount: 2
            dragThreshold: 0
            grabPermissions: PointerHandler.CanTakeOverFromAnything
                             | PointerHandler.ApprovesTakeOverByAnything
            onActiveChanged: {
                if (active)
                    tool.onPinchStart(centroid.position.x, centroid.position.y, activeScale)
                else
                    tool.onPinchEnd()
            }
            onActiveScaleChanged: {
                if (active)
                    tool.onPinchUpdate(centroid.position.x, centroid.position.y, activeScale)
            }
            onCentroidChanged: {
                if (active)
                    tool.onPinchUpdate(centroid.position.x, centroid.position.y, activeScale)
            }
        }
    }

    SelectionContextToolbar {
        anchors.fill: parent
        z: 2
        bar: tool.selectionBar
    }
}
