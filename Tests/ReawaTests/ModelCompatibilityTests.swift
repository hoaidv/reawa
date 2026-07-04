import XCTest
@testable import ReawaApp

final class ModelCompatibilityTests: XCTestCase {
    func testDecodesExistingPythonConnectionShape() throws {
        let json = """
        {
          "connections": [
            {
              "id": "abc-123",
              "name": "My RM2",
              "ip": "10.11.99.1",
              "auto_connect": true,
              "device_config": {
                "output_mode": "ABSOLUTE",
                "scale": 1.25,
                "swap_xy": true,
                "invert_x": false,
                "invert_y": true,
                "absolute": {
                  "region_x": 50,
                  "region_y": 75,
                  "region_width": 500,
                  "region_height": 400,
                  "border_color": "#FF0000",
                  "border_style": "solid",
                  "snap_window_enabled": true,
                  "snapped_window_ref": "Figma"
                }
              }
            }
          ]
        }
        """

        let file = try JSONDecoder().decode(ConnectionFile.self, from: Data(json.utf8))
        XCTAssertEqual(file.connections.count, 1)
        XCTAssertEqual(file.connections[0].id, "abc-123")
        XCTAssertEqual(file.connections[0].deviceConfig.outputMode, .absolute)
        XCTAssertEqual(file.connections[0].deviceConfig.absolute.snappedWindowRef, "Figma")
    }

    func testAbsoluteConfigLocksAspectRatio() {
        var absolute = AbsoluteConfig(regionX: 0, regionY: 0, regionWidth: 640, regionHeight: 640, borderColor: "#000000", borderStyle: "solid", snapWindowEnabled: false, snappedWindowRef: nil)
        absolute.lockAspect()
        XCTAssertEqual(absolute.regionHeight, absolute.regionWidth / RM2.aspect, accuracy: 0.0001)
    }

    func testDecodesNativeStylusOutputMode() throws {
        let json = """
        {
          "connections": [
            {
              "id": "native-123",
              "name": "My RM2",
              "ip": "10.11.99.1",
              "auto_connect": false,
              "device_config": {
                "output_mode": "NATIVE_STYLUS",
                "scale": null,
                "swap_xy": false,
                "invert_x": false,
                "invert_y": false,
                "absolute": {
                  "region_x": 10,
                  "region_y": 10,
                  "region_width": 400,
                  "region_height": 300
                }
              }
            }
          ]
        }
        """

        let file = try JSONDecoder().decode(ConnectionFile.self, from: Data(json.utf8))
        XCTAssertEqual(file.connections.first?.deviceConfig.outputMode, .nativeStylus)
    }

    func testUserNotificationsRequireAppBundle() {
        XCTAssertFalse(NotificationService.supportsUserNotifications(bundleURL: URL(fileURLWithPath: "/tmp/reawa/.build/arm64-apple-macosx/debug/")))
        XCTAssertTrue(NotificationService.supportsUserNotifications(bundleURL: URL(fileURLWithPath: "/Applications/Reawa.app")))
    }

    func testWideSubnetDiscoveryPrefersLocalLinkNeighborhood() {
        let iface = NetworkInterface(
            name: "en7",
            address: "169.254.173.84",
            netmask: "0xffff0000",
            network: "169.254.0.0",
            broadcast: "169.254.255.255",
            prefixLength: 16
        )

        let candidates = NetworkDiscovery.candidateIPs(interface: iface)
        XCTAssertTrue(candidates.prefix(5).contains("169.254.173.83"))
        XCTAssertTrue(candidates.prefix(10).contains("169.254.173.85"))
        XCTAssertFalse(candidates.prefix(20).contains("169.254.0.1"))
    }
}
