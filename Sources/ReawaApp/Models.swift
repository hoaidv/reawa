import CoreGraphics
import Foundation

enum OutputMode: String, Codable, CaseIterable, Sendable {
    case relative = "RELATIVE"
    case absolute = "ABSOLUTE"
    case nativeStylus = "NATIVE_STYLUS"

    var title: String {
        switch self {
        case .relative:
            return "Relative"
        case .absolute:
            return "Absolute"
        case .nativeStylus:
            return "Native Stylus"
        }
    }

    var isMouseEmulation: Bool {
        switch self {
        case .relative, .absolute:
            return true
        case .nativeStylus:
            return false
        }
    }
}

enum NativeStylusStatusKind: String, Sendable {
    case info
    case success
    case warning
    case error
}

struct NativeStylusStatus: Equatable, Sendable {
    let kind: NativeStylusStatusKind
    let message: String
}

enum ConnectionStatus: String, Sendable {
    case offline
    case online
    case connected
    case error
}

enum WindowLifecycle: String, Sendable {
    case closed
    case minimized
    case maximized
    case normal
}

enum LinuxInputEventType: UInt16, Sendable {
    case syn = 0
    case key = 1
    case abs = 3

    var name: String {
        switch self {
        case .syn:
            return "EV_SYN"
        case .key:
            return "EV_KEY"
        case .abs:
            return "EV_ABS"
        }
    }
}

enum LinuxInputCode {
    static let synReport: UInt16 = 0
    static let absX: UInt16 = 0
    static let absY: UInt16 = 1
    static let absPressure: UInt16 = 24
    static let absDistance: UInt16 = 25
    static let absTiltX: UInt16 = 26
    static let absTiltY: UInt16 = 27
    static let btnToolPen: UInt16 = 320
    static let btnTouch: UInt16 = 330
    static let btnStylus: UInt16 = 331
}

func linuxInputTypeName(_ type: UInt16) -> String {
    LinuxInputEventType(rawValue: type)?.name ?? "UNKNOWN(\(type))"
}

func linuxInputCodeName(type: UInt16, code: UInt16) -> String {
    switch LinuxInputEventType(rawValue: type) {
    case .syn:
        if code == LinuxInputCode.synReport {
            return "SYN_REPORT"
        }
    case .key:
        switch code {
        case LinuxInputCode.btnToolPen:
            return "BTN_TOOL_PEN"
        case LinuxInputCode.btnTouch:
            return "BTN_TOUCH"
        case LinuxInputCode.btnStylus:
            return "BTN_STYLUS"
        default:
            break
        }
    case .abs:
        switch code {
        case LinuxInputCode.absX:
            return "ABS_X"
        case LinuxInputCode.absY:
            return "ABS_Y"
        case LinuxInputCode.absPressure:
            return "ABS_PRESSURE"
        case LinuxInputCode.absDistance:
            return "ABS_DISTANCE"
        case LinuxInputCode.absTiltX:
            return "ABS_TILT_X"
        case LinuxInputCode.absTiltY:
            return "ABS_TILT_Y"
        default:
            break
        }
    case nil:
        break
    }
    return "UNKNOWN(\(code))"
}

enum RM2 {
    static let user = "root"
    static let penFile = "/dev/input/event1"
    static let penXMax = 20_967
    static let penYMax = 15_725
    static let aspect = Double(penXMax) / Double(penYMax)
    static let dpi: Double = 2_531
    static let sshKeyBits = 3_072
    static let sshKeyComment = "remarkable-rm2-driver"
    static let eventSize = 16
}

struct AbsoluteConfig: Codable, Equatable, Sendable {
    static let defaultBorderColor = "#3B82F6"

    var regionX: Double = 100
    var regionY: Double = 100
    var regionWidth: Double = 400
    var regionHeight: Double = 400 / RM2.aspect
    var borderColor: String = AbsoluteConfig.defaultBorderColor
    var borderStyle: String = "solid"
    var snapWindowEnabled: Bool = false
    var snappedWindowRef: String? = nil

    init(
        regionX: Double = 100,
        regionY: Double = 100,
        regionWidth: Double = 400,
        regionHeight: Double = 400 / RM2.aspect,
        borderColor: String = AbsoluteConfig.defaultBorderColor,
        borderStyle: String = "solid",
        snapWindowEnabled: Bool = false,
        snappedWindowRef: String? = nil
    ) {
        self.regionX = regionX
        self.regionY = regionY
        self.regionWidth = regionWidth
        self.regionHeight = regionHeight
        self.borderColor = borderColor
        self.borderStyle = borderStyle
        self.snapWindowEnabled = snapWindowEnabled
        self.snappedWindowRef = snappedWindowRef
        lockAspect()
    }

    mutating func lockAspect() {
        regionHeight = regionWidth / RM2.aspect
    }

    var rect: CGRect {
        CGRect(x: regionX, y: regionY, width: regionWidth, height: regionHeight)
    }

    private enum CodingKeys: String, CodingKey {
        case regionX = "region_x"
        case regionY = "region_y"
        case regionWidth = "region_width"
        case regionHeight = "region_height"
        case borderColor = "border_color"
        case borderStyle = "border_style"
        case snapWindowEnabled = "snap_window_enabled"
        case snappedWindowRef = "snapped_window_ref"
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        regionX = try container.decodeIfPresent(Double.self, forKey: .regionX) ?? 100
        regionY = try container.decodeIfPresent(Double.self, forKey: .regionY) ?? 100
        regionWidth = try container.decodeIfPresent(Double.self, forKey: .regionWidth) ?? 400
        regionHeight = try container.decodeIfPresent(Double.self, forKey: .regionHeight) ?? (400 / RM2.aspect)
        borderColor = try container.decodeIfPresent(String.self, forKey: .borderColor) ?? AbsoluteConfig.defaultBorderColor
        borderStyle = try container.decodeIfPresent(String.self, forKey: .borderStyle) ?? "solid"
        snapWindowEnabled = try container.decodeIfPresent(Bool.self, forKey: .snapWindowEnabled) ?? false
        snappedWindowRef = try container.decodeIfPresent(String.self, forKey: .snappedWindowRef)
        lockAspect()
    }
}

struct DeviceConfig: Codable, Equatable, Sendable {
    var outputMode: OutputMode = .relative
    var scale: Double? = nil
    var swapXY = false
    var invertX = false
    var invertY = false
    var absolute = AbsoluteConfig()

    private enum CodingKeys: String, CodingKey {
        case outputMode = "output_mode"
        case scale
        case swapXY = "swap_xy"
        case invertX = "invert_x"
        case invertY = "invert_y"
        case absolute
    }
}

struct Connection: Codable, Identifiable, Equatable, Sendable {
    var id: String = UUID().uuidString
    var name: String
    var ip: String
    var autoConnect = false
    var deviceConfig = DeviceConfig()

    private enum CodingKeys: String, CodingKey {
        case id
        case name
        case ip
        case autoConnect = "auto_connect"
        case deviceConfig = "device_config"
    }
}

struct ConnectionFile: Codable, Sendable {
    var connections: [Connection]
}

struct PenRawEvent: Equatable, Sendable {
    let tvSec: UInt32
    let tvUsec: UInt32
    let type: UInt16
    let code: UInt16
    let value: Int

    var typeName: String {
        linuxInputTypeName(type)
    }

    var codeName: String {
        linuxInputCodeName(type: type, code: code)
    }

    var rawDataText: String {
        "\(typeName) \(codeName) \(value)"
    }

    var capabilityLabels: [String] {
        switch (type, code) {
        case (LinuxInputEventType.abs.rawValue, LinuxInputCode.absPressure):
            return ["ABS_PRESSURE", "Pressure"]
        case (LinuxInputEventType.abs.rawValue, LinuxInputCode.absDistance):
            return ["ABS_DISTANCE", "Distance"]
        case (LinuxInputEventType.abs.rawValue, LinuxInputCode.absTiltX),
             (LinuxInputEventType.abs.rawValue, LinuxInputCode.absTiltY):
            return [codeName, "Tilt"]
        case (LinuxInputEventType.key.rawValue, LinuxInputCode.btnStylus):
            return ["BTN_STYLUS", "Stylus button"]
        case (LinuxInputEventType.key.rawValue, LinuxInputCode.btnTouch):
            return ["BTN_TOUCH", "Touch"]
        case (LinuxInputEventType.key.rawValue, LinuxInputCode.btnToolPen):
            return ["BTN_TOOL_PEN", "Proximity"]
        case (LinuxInputEventType.abs.rawValue, LinuxInputCode.absX):
            return ["ABS_X", "Position"]
        case (LinuxInputEventType.abs.rawValue, LinuxInputCode.absY):
            return ["ABS_Y", "Position"]
        default:
            return [codeName]
        }
    }
}

struct PenStateSnapshot: Equatable, Sendable {
    var x: Int?
    var y: Int?
    var pressure: Int?
    var distance: Int?
    var tiltX: Int?
    var tiltY: Int?
    var touching = false
    var inProximity = false
    var stylusButton = false
}

struct PenFrame: Equatable, Sendable {
    let tvSec: UInt32
    let tvUsec: UInt32
    let x: Int
    let y: Int
    let pressure: Int?
    let touching: Bool
    let inProximity: Bool
    let stylusButton: Bool
    let distance: Int?
    let tiltX: Int?
    let tiltY: Int?
    let rawEvents: [PenRawEvent]

    init(
        tvSec: UInt32,
        tvUsec: UInt32,
        x: Int,
        y: Int,
        pressure: Int?,
        touching: Bool,
        inProximity: Bool,
        stylusButton: Bool = false,
        distance: Int? = nil,
        tiltX: Int? = nil,
        tiltY: Int? = nil,
        rawEvents: [PenRawEvent] = []
    ) {
        self.tvSec = tvSec
        self.tvUsec = tvUsec
        self.x = x
        self.y = y
        self.pressure = pressure
        self.touching = touching
        self.inProximity = inProximity
        self.stylusButton = stylusButton
        self.distance = distance
        self.tiltX = tiltX
        self.tiltY = tiltY
        self.rawEvents = rawEvents
    }

    var snapshot: PenStateSnapshot {
        PenStateSnapshot(
            x: x,
            y: y,
            pressure: pressure,
            distance: distance,
            tiltX: tiltX,
            tiltY: tiltY,
            touching: touching,
            inProximity: inProximity,
            stylusButton: stylusButton
        )
    }
}

struct WindowInfo: Equatable, Sendable {
    let pid: pid_t
    let windowNumber: Int
    let name: String
    let bounds: CGRect
}
