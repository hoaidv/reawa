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
        XCTAssertEqual(frames[0], PenFrame(tvSec: 1, tvUsec: 2, x: 1234, y: 5678, pressure: nil, touching: true, inProximity: true))
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
