// Traceability (ADLC iter-000)
// @implements SRS-RW-36
// @implements SRS-RW-39

import AppKit
import Foundation

enum ProcessError: Error, LocalizedError {
    case nonZeroExit(code: Int32, stderr: String)
    case launchFailed(String)

    var errorDescription: String? {
        switch self {
        case let .nonZeroExit(code, stderr):
            let trimmed = stderr.trimmingCharacters(in: .whitespacesAndNewlines)
            return trimmed.isEmpty ? "Command failed with exit code \(code)." : trimmed
        case let .launchFailed(message):
            return message
        }
    }
}

enum ProcessRunner {
    @discardableResult
    static func run(
        launchPath: String,
        arguments: [String],
        environment: [String: String] = [:],
        currentDirectoryURL: URL? = nil
    ) throws -> String {
        let process = Process()
        let stdout = Pipe()
        let stderr = Pipe()
        process.executableURL = URL(fileURLWithPath: launchPath)
        process.arguments = arguments
        process.standardOutput = stdout
        process.standardError = stderr
        if !environment.isEmpty {
            process.environment = ProcessInfo.processInfo.environment.merging(environment, uniquingKeysWith: { _, new in new })
        }
        process.currentDirectoryURL = currentDirectoryURL

        do {
            try process.run()
        } catch {
            throw ProcessError.launchFailed(error.localizedDescription)
        }
        process.waitUntilExit()

        let output = String(data: stdout.fileHandleForReading.readDataToEndOfFile(), encoding: .utf8) ?? ""
        let errorOutput = String(data: stderr.fileHandleForReading.readDataToEndOfFile(), encoding: .utf8) ?? ""
        guard process.terminationStatus == 0 else {
            throw ProcessError.nonZeroExit(code: process.terminationStatus, stderr: errorOutput)
        }
        return output
    }
}

func primaryScreenHeight() -> CGFloat {
    let screens = NSScreen.screens
    if let first = screens.first {
        return first.frame.height
    }
    return NSScreen.main?.frame.height ?? 0
}

func cgRectToCocoa(_ rect: CGRect) -> CGRect {
    CGRect(x: rect.origin.x, y: primaryScreenHeight() - rect.origin.y - rect.height, width: rect.width, height: rect.height)
}

func cgPointToCocoa(_ point: CGPoint) -> CGPoint {
    CGPoint(x: point.x, y: primaryScreenHeight() - point.y)
}

func desktopBoundsCocoa() -> CGRect {
    let screens = NSScreen.screens
    guard let first = screens.first else {
        return NSScreen.main?.frame ?? .zero
    }

    return screens.dropFirst().reduce(first.frame) { partial, screen in
        partial.union(screen.frame)
    }
}

func desktopBoundsQuartz() -> CGRect {
    CGRect(x: desktopBoundsCocoa().minX, y: desktopBoundsCocoa().minY, width: desktopBoundsCocoa().width, height: desktopBoundsCocoa().height)
}

func clampValue<T: Comparable>(_ value: T, min lower: T, max upper: T) -> T {
    max(lower, min(upper, value))
}

func shellEscapeSingleQuotes(_ string: String) -> String {
    string.replacingOccurrences(of: "'", with: "'\"'\"'")
}

func hexColorComponents(_ hex: String) -> (CGFloat, CGFloat, CGFloat) {
    let cleaned = hex.trimmingCharacters(in: CharacterSet(charactersIn: "#"))
    guard cleaned.count == 6, let value = Int(cleaned, radix: 16) else {
        return (0.23, 0.51, 0.96)
    }
    let r = CGFloat((value >> 16) & 0xFF) / 255
    let g = CGFloat((value >> 8) & 0xFF) / 255
    let b = CGFloat(value & 0xFF) / 255
    return (r, g, b)
}

extension CGRect {
    var center: CGPoint {
        CGPoint(x: midX, y: midY)
    }
}
