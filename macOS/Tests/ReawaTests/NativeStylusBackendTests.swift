import XCTest
@testable import ReawaApp

final class NativeStylusBackendTests: XCTestCase {
    func testNativeStylusBackendErrorsAreActionable() {
        XCTAssertEqual(
            NativeStylusBackendError.appBundleRequired.errorDescription,
            "Native Stylus cannot run from `swift run`. Launch a signed `.app` bundle instead."
        )
        XCTAssertEqual(
            NativeStylusBackendError.missingVirtualHIDEntitlement(executable: "/tmp/reawa").errorDescription,
            "The current executable lacks `com.apple.developer.hid.virtual.device`: /tmp/reawa"
        )
    }

    func testNativeStylusReportEncodesSwitchesAndPressure() {
        let config = DeviceConfig(
            outputMode: .nativeStylus,
            scale: nil,
            swapXY: false,
            invertX: false,
            invertY: false,
            absolute: AbsoluteConfig()
        )
        let frame = PenFrame(
            tvSec: 0,
            tvUsec: 0,
            x: RM2.penXMax,
            y: RM2.penYMax,
            pressure: 9_999,
            touching: true,
            inProximity: true,
            stylusButton: true,
            distance: nil,
            tiltX: 200,
            tiltY: -200
        )

        let report = NativeStylusReport(frame: frame, config: config)

        XCTAssertEqual(report.switches, 0b0000_0111)
        XCTAssertEqual(report.x, UInt16(NativeStylusReport.coordinateMax))
        XCTAssertEqual(report.y, UInt16(NativeStylusReport.coordinateMax))
        XCTAssertEqual(report.pressure, UInt16(NativeStylusReport.pressureMax))
        XCTAssertEqual(report.tiltX, 127)
        XCTAssertEqual(report.tiltY, -127)
        XCTAssertEqual(report.data.count, 10)
        XCTAssertEqual(report.data.first, NativeStylusReport.reportID)
    }

    func testNativeStylusReportUsesTabletOrientationMapping() {
        let config = DeviceConfig(
            outputMode: .nativeStylus,
            scale: nil,
            swapXY: true,
            invertX: true,
            invertY: false,
            absolute: AbsoluteConfig()
        )
        let frame = PenFrame(
            tvSec: 0,
            tvUsec: 0,
            x: 1_000,
            y: 2_000,
            pressure: 100,
            touching: false,
            inProximity: true,
            stylusButton: false,
            distance: nil,
            tiltX: 0,
            tiltY: 0
        )

        let report = NativeStylusReport(frame: frame, config: config)
        let expectedMapped = PenCoordinateMapper.mapPenCoordinates(x: frame.x, y: frame.y, config: config)

        XCTAssertGreaterThan(report.x, report.y)
        XCTAssertEqual(
            report.x,
            UInt16((Double(expectedMapped.x) / Double(RM2.penXMax) * Double(NativeStylusReport.coordinateMax)).rounded())
        )
        XCTAssertEqual(
            report.y,
            UInt16((Double(expectedMapped.y) / Double(RM2.penYMax) * Double(NativeStylusReport.coordinateMax)).rounded())
        )
    }
}
