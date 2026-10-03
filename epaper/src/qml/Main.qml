import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtLearn 1.0

Window {
    id: root
    width: Screen.width
    height: Screen.height
    visible: true


    SketchCanvas {
        id: canvas
        x: 0
        y: 0
        width: root.width
        height: root.height

        PointHandler {
            acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
            target: null
            onActiveChanged: {
                if (active)
                    canvas.beginStroke(point.position.x,point.position.y)
            }
            onPointChanged: {
                if (active) {
                    canvas.extendStroke(point.position.x,point.position.y)
                }
            }
        }
    }

    Component.onCompleted: EpaperBridge.attachPenModeRegion(canvas)
}

