import XCTest
@testable import ReawaApp

final class PenFrameParserTests: XCTestCase {
    func testParserProducesFrameOnSynReport() {
        var parser = PenFrameParser()
        let stream = event(type: 3, code: 0, value: 1234)
            + event(type: 3, code: 1, value: 5678)
            + event(type: 1, code: 320, value: 1)
            + event(type: 1, code: 330, value: 1)
            + event(type: 0, code: 0, value: 0)

        let frames = parser.append(stream)
        XCTAssertEqual(frames.count, 1)
        XCTAssertEqual(frames[0].tvSec, 1)
        XCTAssertEqual(frames[0].tvUsec, 2)
        XCTAssertEqual(frames[0].x, 1234)
        XCTAssertEqual(frames[0].y, 5678)
        XCTAssertEqual(frames[0].pressure, nil)
        XCTAssertTrue(frames[0].touching)
        XCTAssertTrue(frames[0].inProximity)
        XCTAssertEqual(frames[0].rawEvents.map(\.codeName), ["ABS_X", "ABS_Y", "BTN_TOOL_PEN", "BTN_TOUCH", "SYN_REPORT"])
    }

    func testParserTracksExtendedPenStateAndRawEvents() {
        var parser = PenFrameParser()
        var observed: [(String, PenGestureState?)] = []
        parser.onRawEvent = { rawEvent, _, gestureState in
            observed.append((rawEvent.rawDataText, gestureState))
        }

        let stream = event(type: 1, code: 320, value: 1)
            + event(type: 3, code: 0, value: 73)
            + event(type: 3, code: 1, value: 110)
            + event(type: 1, code: 330, value: 1)
            + event(type: 3, code: 25, value: 5)
            + event(type: 3, code: 26, value: -2)
            + event(type: 3, code: 27, value: 4)
            + event(type: 1, code: 331, value: 1)
            + event(type: 0, code: 0, value: 0)

        let frames = parser.append(stream)
        XCTAssertEqual(frames.count, 1)
        XCTAssertEqual(frames[0].x, 73)
        XCTAssertEqual(frames[0].y, 110)
        XCTAssertEqual(frames[0].distance, 5)
        XCTAssertEqual(frames[0].tiltX, -2)
        XCTAssertEqual(frames[0].tiltY, 4)
        XCTAssertTrue(frames[0].stylusButton)
        XCTAssertEqual(
            frames[0].rawEvents.map(\.codeName),
            ["BTN_TOOL_PEN", "ABS_X", "ABS_Y", "BTN_TOUCH", "ABS_DISTANCE", "ABS_TILT_X", "ABS_TILT_Y", "BTN_STYLUS", "SYN_REPORT"]
        )
        XCTAssertEqual(
            observed.map(\.0),
            ["EV_KEY BTN_TOOL_PEN 1", "EV_ABS ABS_X 73", "EV_ABS ABS_Y 110", "EV_KEY BTN_TOUCH 1", "EV_ABS ABS_DISTANCE 5", "EV_ABS ABS_TILT_X -2", "EV_ABS ABS_TILT_Y 4", "EV_KEY BTN_STYLUS 1"]
        )
        XCTAssertEqual(
            observed.map(\.1),
            [.start, .move, .move, .start, .move, .move, .move, nil]
        )
    }

    func testParserRecognizesPenOutGesture() {
        var parser = PenFrameParser()
        var gestures: [PenGestureState?] = []
        parser.onRawEvent = { _, _, gestureState in
            gestures.append(gestureState)
        }

        let stream = event(type: 3, code: 0, value: 73)
            + event(type: 3, code: 1, value: 110)
            + event(type: 1, code: 320, value: 1)
            + event(type: 0, code: 0, value: 0)
            + event(type: 1, code: 320, value: 0)
            + event(type: 0, code: 0, value: 0)

        let frames = parser.append(stream)
        XCTAssertEqual(frames.count, 2)
        XCTAssertEqual(gestures.last!, .out)
        XCTAssertFalse(frames.last!.inProximity)
    }

    private func event(type: UInt16, code: UInt16, value: Int32) -> Data {
        var data = Data()
        var tvSec = UInt32(1).littleEndian
        var tvUsec = UInt32(2).littleEndian
        var type = type.littleEndian
        var code = code.littleEndian
        var value = value.littleEndian
        withUnsafeBytes(of: &tvSec) { data.append(contentsOf: $0) }
        withUnsafeBytes(of: &tvUsec) { data.append(contentsOf: $0) }
        withUnsafeBytes(of: &type) { data.append(contentsOf: $0) }
        withUnsafeBytes(of: &code) { data.append(contentsOf: $0) }
        withUnsafeBytes(of: &value) { data.append(contentsOf: $0) }
        return data
    }
}
