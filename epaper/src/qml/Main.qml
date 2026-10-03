import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtLearn 1.0

NativeWindow {
    id: root
    width: Screen.width
    height: Screen.height
    visible: true


    NativeCanvas {
        id: canvas
        x: 0
        y: 0
        width: root.width
        height: root.height
        waveform: NativeCanvas.PenMode
        batchWindowMs: 8
        penWidth: 4

        PointHandler {
            acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Mouse
            target: null
            onActiveChanged: {
                if (active)
                    canvas.beginStroke(point.position.x, point.position.y)
                else
                    canvas.endStroke()
            }
            onPointChanged: {
                if (active)
                    canvas.extendStroke(point.position.x, point.position.y)
            }
        }
    }

  

    StylusHandler {
        onStylusPress: (x, y) => canvas.beginStroke(x, y)
        onStylusMove: (x, y) => canvas.extendStroke(x, y)
        onStylusRelease: canvas.endStroke()
    }
}

