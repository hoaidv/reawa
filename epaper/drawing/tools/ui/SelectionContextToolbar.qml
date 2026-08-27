import QtQuick

// Selection overlay: action strip + visual-only resize knobs (HitTarget lives on the hub).
// @implements [SRS-EP-12]
Item {
    id: root
    property var bar

    visible: bar !== null

    Repeater {
        model: bar ? bar.handleCount : 0
        delegate: Item {
            readonly property var _r: bar.selectionBounds
            readonly property real _h: bar.handleSize
            readonly property real _hit: Math.max(_h, 64)
            readonly property point _pt: {
                var r = _r
                var n = bar.handleCount
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
            width: _hit
            height: _hit
            x: _pt.x - _hit / 2
            y: _pt.y - _hit / 2
            visible: bar.handleCount > 0 && _r.width > 0 && _r.height > 0
            z: 22
            Rectangle {
                anchors.centerIn: parent
                width: _h
                height: _h
                color: "white"
                border.color: "black"
                border.width: 2
            }
        }
    }

    Row {
        id: strip
        readonly property real tile: 64
        x: {
            if (!bar)
                return 0
            var r = bar.selectionBounds
            return r.x + r.width / 2 - width / 2
        }
        y: {
            if (!bar)
                return 0
            var r = bar.selectionBounds
            var knob = bar.handleCount > 0 ? bar.handleSize * 0.5 : 0
            return r.y + r.height + knob + 16
        }
        visible: bar && bar.selectionBounds.width > 0 && bar.actions.count > 0
        spacing: 0

        Repeater {
            model: bar ? bar.actions : null
            delegate: Rectangle {
                width: strip.tile
                height: strip.tile
                color: "white"
                border.color: "black"
                border.width: model.enabled ? 2 : 1

                Image {
                    anchors.centerIn: parent
                    width: parent.width * 0.62
                    height: parent.height * 0.62
                    fillMode: Image.PreserveAspectFit
                    smooth: false
                    visible: model.icon.length > 0
                    source: model.icon
                }
                Text {
                    anchors.fill: parent
                    anchors.margins: 4
                    visible: model.icon.length === 0
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    color: "black"
                    text: model.label
                }
                TapHandler {
                    acceptedDevices: PointerDevice.Stylus | PointerDevice.TouchScreen | PointerDevice.Mouse
                    acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Finger | PointerDevice.Generic
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    grabPermissions: PointerHandler.CanTakeOverFromItems
                                     | PointerHandler.ApprovesCancellation
                    enabled: model.enabled
                    onTapped: bar.trigger(model.actionId)
                }
            }
        }
    }

    Text {
        visible: bar && bar.refuseReason.length > 0
        x: strip.visible ? strip.x : 24
        y: strip.visible ? strip.y + strip.height + 6 : 80
        font.pixelSize: 16
        color: "black"
        text: bar ? bar.refuseReason : ""
    }

    Rectangle {
        visible: bar && bar.manipUnavailable.length > 0
        x: bar ? bar.manipUnavailableRect.x : 0
        y: bar ? bar.manipUnavailableRect.y : 0
        width: bar ? Math.max(220, bar.manipUnavailableRect.width) : 220
        height: bar ? Math.max(36, bar.manipUnavailableRect.height) : 36
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
            text: bar ? bar.manipUnavailable : ""
        }
    }
}
