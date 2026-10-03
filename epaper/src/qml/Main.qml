import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtLearn 1.0
import epaper 1.0 // URI from qmlRegisterSingletonInstance; exposes EpaperBridgeInstance

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
                    canvas.beginStroke(point.position.x, point.position.y)
                else
                    canvas.endStroke()
            }
            onPointChanged: {
                if (active) {
                    canvas.extendStroke(point.position.x,point.position.y)
                }
            }
        }
    }

    // Tags this canvas so update() uses the pen waveform.
    // Without the region, the panel's default refresh draws the stroke dashed.
    Component.onCompleted: EpaperBridgeInstance.attachPenModeRegion(canvas)
}

