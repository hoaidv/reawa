import CoreGraphics
import Foundation

enum OutputMode: String, Codable, CaseIterable, Sendable {
    case relative = "RELATIVE"
    case absolute = "ABSOLUTE"
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

struct PenFrame: Equatable, Sendable {
    let tvSec: UInt32
    let tvUsec: UInt32
    let x: Int
    let y: Int
    let pressure: Int?
    let touching: Bool
    let inProximity: Bool
}

struct WindowInfo: Equatable, Sendable {
    let pid: pid_t
    let windowNumber: Int
    let name: String
    let bounds: CGRect
}
