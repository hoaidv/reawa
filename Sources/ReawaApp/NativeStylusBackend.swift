// Traceability (ADLC iter-000)
// @implements SRS-RW-51
// @implements SRS-RW-52
// @implements SRS-RW-53
// @implements SRS-RW-54
// @implements SRS-RW-55
// @implements SRS-RW-56
// @implements SRS-RW-57
// @implements SRS-RW-60
// @implements SRS-RW-61

import Foundation
import Security

#if canImport(CoreHID)
import CoreHID
#endif

#if canImport(IOKit.hid)
import IOKit.hid
#endif

enum NativeStylusBackendError: LocalizedError, Sendable {
    case unsupportedOSVersion
    case appBundleRequired
    case missingVirtualHIDEntitlement(executable: String)
    case accessibilityDenied
    case virtualDeviceUnavailable
    case frameworkUnavailable

    var errorDescription: String? {
        switch self {
        case .unsupportedOSVersion:
            return "Native Stylus requires macOS 15 or newer."
        case .appBundleRequired:
            return "Native Stylus cannot run from `swift run`. Launch a signed `.app` bundle instead."
        case let .missingVirtualHIDEntitlement(executable):
            return "The current executable lacks `com.apple.developer.hid.virtual.device`: \(executable)"
        case .accessibilityDenied:
            return "Native Stylus needs Accessibility permission to post stylus events."
        case .virtualDeviceUnavailable:
            return "Native Stylus could not create a virtual pen device. The virtual HID entitlement may be missing."
        case .frameworkUnavailable:
            return "CoreHID is not available in this build environment."
        }
    }
}

struct NativeStylusReport: Equatable {
    static let reportID: UInt8 = 1
    static let coordinateMax = 32_767
    static let pressureMax = 4_095

    let switches: UInt8
    let x: UInt16
    let y: UInt16
    let pressure: UInt16
    let tiltX: Int8
    let tiltY: Int8

    static let reportDescriptor = Data([
        0x05, 0x0D,
        0x09, 0x02,
        0xA1, 0x01,
        0x85, reportID,
        0x09, 0x20,
        0xA1, 0x00,
        0x09, 0x42,
        0x09, 0x44,
        0x09, 0x32,
        0x15, 0x00,
        0x25, 0x01,
        0x75, 0x01,
        0x95, 0x03,
        0x81, 0x02,
        0x75, 0x05,
        0x95, 0x01,
        0x81, 0x03,
        0x05, 0x01,
        0x09, 0x30,
        0x09, 0x31,
        0x16, 0x00, 0x00,
        0x26, 0xFF, 0x7F,
        0x75, 0x10,
        0x95, 0x02,
        0x81, 0x02,
        0x05, 0x0D,
        0x09, 0x30,
        0x15, 0x00,
        0x26, 0xFF, 0x0F,
        0x75, 0x10,
        0x95, 0x01,
        0x81, 0x02,
        0x09, 0x3D,
        0x09, 0x3E,
        0x15, 0x81,
        0x25, 0x7F,
        0x75, 0x08,
        0x95, 0x02,
        0x81, 0x02,
        0xC0,
        0xC0,
    ])

    init(frame: PenFrame, config: DeviceConfig) {
        let mapped = PenCoordinateMapper.mapPenCoordinates(x: frame.x, y: frame.y, config: config)
        let tipSwitch = frame.touching ? UInt8(1 << 0) : 0
        let barrelSwitch = frame.stylusButton ? UInt8(1 << 1) : 0
        let inRange = frame.inProximity ? UInt8(1 << 2) : 0

        switches = tipSwitch | barrelSwitch | inRange
        x = Self.normalizedCoordinate(Double(mapped.x), maxInput: Double(RM2.penXMax))
        y = Self.normalizedCoordinate(Double(mapped.y), maxInput: Double(RM2.penYMax))
        pressure = Self.normalizedPressure(frame.pressure)
        tiltX = Self.normalizedTilt(frame.tiltX)
        tiltY = Self.normalizedTilt(frame.tiltY)
    }

    var data: Data {
        var bytes = Data([Self.reportID, switches])
        Self.appendLE(x, to: &bytes)
        Self.appendLE(y, to: &bytes)
        Self.appendLE(pressure, to: &bytes)
        bytes.append(UInt8(bitPattern: tiltX))
        bytes.append(UInt8(bitPattern: tiltY))
        return bytes
    }

    private static func appendLE(_ value: UInt16, to data: inout Data) {
        var little = value.littleEndian
        withUnsafeBytes(of: &little) { data.append(contentsOf: $0) }
    }

    private static func normalizedCoordinate(_ value: Double, maxInput: Double) -> UInt16 {
        guard maxInput > 0 else {
            return 0
        }
        let normalized = (value / maxInput) * Double(coordinateMax)
        return UInt16(clamp(Int(normalized.rounded()), min: 0, max: coordinateMax))
    }

    private static func normalizedPressure(_ rawPressure: Int?) -> UInt16 {
        guard let rawPressure, rawPressure > 0 else {
            return 0
        }
        return UInt16(clamp(rawPressure, min: 0, max: pressureMax))
    }

    private static func normalizedTilt(_ rawTilt: Int?) -> Int8 {
        Int8(clamp(rawTilt ?? 0, min: -127, max: 127))
    }
}

private protocol NativeStylusVirtualDeviceDispatching: Sendable {
    func activate() async
    func dispatch(_ report: Data) async throws
}

private enum NativeStylusEnvironment {
    static func startupError() -> NativeStylusBackendError? {
        guard isRunningFromAppBundle else {
            return .appBundleRequired
        }
        guard hasVirtualHIDEntitlement else {
            return .missingVirtualHIDEntitlement(executable: executablePath)
        }
        return nil
    }

    static var isRunningFromAppBundle: Bool {
        Bundle.main.bundleURL.pathExtension == "app"
    }

    static var executablePath: String {
        Bundle.main.executableURL?.path ?? CommandLine.arguments.first ?? "(unknown executable)"
    }

    static var hasVirtualHIDEntitlement: Bool {
        guard let task = SecTaskCreateFromSelf(nil) else {
            return false
        }
        let entitlementKey = "com.apple.developer.hid.virtual.device" as CFString
        let value = SecTaskCopyValueForEntitlement(task, entitlementKey, nil)
        return value as? Bool == true
    }
}

private enum NativeStylusAccess {
    static func ensurePostEventPermission() -> Bool {
#if canImport(IOKit.hid)
        guard #available(macOS 10.15, *) else {
            return false
        }

        let grantedRawValue: UInt32 = 0
        let current = IOHIDCheckAccess(kIOHIDRequestTypePostEvent)
        if current.rawValue == grantedRawValue {
            return true
        }
        return IOHIDRequestAccess(kIOHIDRequestTypePostEvent)
#else
        return false
#endif
    }
}

final class NativeStylusBackend: PenOutputBackend, @unchecked Sendable {
    private let logger: AppLogger
    private let onStatus: @Sendable (NativeStylusStatus) -> Void
    private let device: any NativeStylusVirtualDeviceDispatching
    private let stateLock = NSLock()

    private var config: DeviceConfig
    private var isClosed = false
    private var lastStatus: NativeStylusStatus?

    init(
        config: DeviceConfig,
        logger: AppLogger,
        onStatus: @escaping @Sendable (NativeStylusStatus) -> Void
    ) throws {
        self.config = config
        self.logger = logger
        self.onStatus = onStatus

        guard #available(macOS 15.0, *) else {
            throw NativeStylusBackendError.unsupportedOSVersion
        }
        if let startupError = NativeStylusEnvironment.startupError() {
            throw startupError
        }
        guard NativeStylusAccess.ensurePostEventPermission() else {
            throw NativeStylusBackendError.accessibilityDenied
        }
        device = try Self.makeVirtualDevice()

        publishStatus(.init(kind: .info, message: "Activating Native Stylus virtual device…"))
        logger.logAsync("Starting Native Stylus backend.", level: "info", category: .mode)

        Task { [weak self] in
            await self?.device.activate()
            self?.publishStatus(.init(kind: .success, message: "Native Stylus virtual device is active."))
            self?.logger.logAsync("Native Stylus backend active.", level: "info", category: .mode)
        }
    }

    func handle(frame: PenFrame) {
        let report: NativeStylusReport? = stateLock.withLock {
            guard !isClosed else {
                return nil
            }
            return NativeStylusReport(frame: frame, config: config)
        }
        guard let report else {
            return
        }

        Task { [weak self] in
            guard let self, !self.isBackendClosed else {
                return
            }
            do {
                try await self.device.dispatch(report.data)
            } catch {
                let status = NativeStylusStatus(
                    kind: .error,
                    message: "Native Stylus failed to submit a report: \(error.localizedDescription)"
                )
                self.publishStatus(status)
                self.logger.logAsync(status.message, level: "error", category: .mode)
            }
        }
    }

    func cleanup() {
        stateLock.withLock {
            isClosed = true
        }
    }

    func updateConfig(_ config: DeviceConfig) {
        stateLock.withLock {
            self.config = config
        }
    }

    private var isBackendClosed: Bool {
        stateLock.withLock { isClosed }
    }

    private func publishStatus(_ status: NativeStylusStatus) {
        let shouldPublish = stateLock.withLock {
            guard status != lastStatus else {
                return false
            }
            lastStatus = status
            return true
        }
        guard shouldPublish else {
            return
        }
        onStatus(status)
    }

    private static func makeVirtualDevice() throws -> any NativeStylusVirtualDeviceDispatching {
#if canImport(CoreHID)
        guard #available(macOS 15.0, *) else {
            throw NativeStylusBackendError.unsupportedOSVersion
        }
        return try CoreHIDNativeStylusDevice()
#else
        throw NativeStylusBackendError.frameworkUnavailable
#endif
    }
}

#if canImport(CoreHID)
@available(macOS 15.0, *)
private final class NativeStylusDeviceDelegate: NSObject, HIDVirtualDeviceDelegate {
    func hidVirtualDevice(
        _ device: HIDVirtualDevice,
        receivedSetReportRequestOfType type: HIDReportType,
        id: HIDReportID?,
        data: Data
    ) throws {}

    func hidVirtualDevice(
        _ device: HIDVirtualDevice,
        receivedGetReportRequestOfType type: HIDReportType,
        id: HIDReportID?,
        maxSize: size_t
    ) throws -> Data {
        Data()
    }
}

@available(macOS 15.0, *)
private actor CoreHIDNativeStylusDevice: NativeStylusVirtualDeviceDispatching {
    private let delegate = NativeStylusDeviceDelegate()
    private let device: HIDVirtualDevice

    init() throws {
        let properties = HIDVirtualDevice.Properties(
            descriptor: NativeStylusReport.reportDescriptor,
            vendorID: 0x23AB
        )
        guard let device = HIDVirtualDevice(properties: properties) else {
            throw NativeStylusBackendError.virtualDeviceUnavailable
        }
        self.device = device
    }

    func activate() async {
        await device.activate(delegate: delegate)
    }

    func dispatch(_ report: Data) async throws {
        try await device.dispatchInputReport(data: report, timestamp: SuspendingClock.now)
    }
}
#endif

private extension NSLock {
    func withLock<T>(_ work: () -> T) -> T {
        lock()
        defer { unlock() }
        return work()
    }
}

private func clamp<T: Comparable>(_ value: T, min lowerBound: T, max upperBound: T) -> T {
    Swift.max(lowerBound, Swift.min(value, upperBound))
}
