// Traceability (ADLC iter-000)
// @implements SRS-RW-38
// @implements SRS-RW-42
// @implements SRS-RW-49

import AppKit
import ApplicationServices
import CoreGraphics
import Foundation

private let frameTolerance: CGFloat = 6
private let stageAreaRatio: CGFloat = 0.5

private func axCopyAttribute(_ element: AXUIElement, _ attribute: CFString) -> CFTypeRef? {
    var value: CFTypeRef?
    let error = AXUIElementCopyAttributeValue(element, attribute, &value)
    guard error == .success else {
        return nil
    }
    return value
}

private func axBool(_ value: CFTypeRef?) -> Bool? {
    switch value {
    case let number as NSNumber:
        return number.boolValue
    case let boolValue as Bool:
        return boolValue
    default:
        return nil
    }
}

private func axPoint(_ value: CFTypeRef?) -> CGPoint? {
    guard let value else {
        return nil
    }
    let axValue = unsafeDowncast(value, to: AXValue.self)
    guard AXValueGetType(axValue) == .cgPoint else {
        return nil
    }
    var point = CGPoint.zero
    guard AXValueGetValue(axValue, .cgPoint, &point) else {
        return nil
    }
    return point
}

private func axSize(_ value: CFTypeRef?) -> CGSize? {
    guard let value else {
        return nil
    }
    let axValue = unsafeDowncast(value, to: AXValue.self)
    guard AXValueGetType(axValue) == .cgSize else {
        return nil
    }
    var size = CGSize.zero
    guard AXValueGetValue(axValue, .cgSize, &size) else {
        return nil
    }
    return size
}

func axWindowFrame(_ element: AXUIElement) -> CGRect? {
    guard let point = axPoint(axCopyAttribute(element, kAXPositionAttribute as CFString)),
          let size = axSize(axCopyAttribute(element, kAXSizeAttribute as CFString))
    else {
        return nil
    }
    return CGRect(origin: point, size: size)
}

func listOnscreenWindows() -> [WindowInfo] {
    let options: CGWindowListOption = [.optionOnScreenOnly, .excludeDesktopElements]
    guard let raw = CGWindowListCopyWindowInfo(options, kCGNullWindowID) as? [[String: Any]] else {
        return []
    }

    let ownPID = ProcessInfo.processInfo.processIdentifier
    return raw.compactMap { entry in
        guard let layer = entry[kCGWindowLayer as String] as? Int, layer == 0,
              let pid = entry[kCGWindowOwnerPID as String] as? pid_t, pid != ownPID,
              let number = entry[kCGWindowNumber as String] as? Int,
              let boundsDict = entry[kCGWindowBounds as String] as? NSDictionary,
              let bounds = CGRect(dictionaryRepresentation: boundsDict),
              bounds.width >= 40,
              bounds.height >= 40
        else {
            return nil
        }

        let name = (entry[kCGWindowName as String] as? String)
            ?? (entry[kCGWindowOwnerName as String] as? String)
            ?? ""
        return WindowInfo(pid: pid, windowNumber: number, name: name, bounds: bounds)
    }
}

func windowUnderPoint(_ point: CGPoint) -> WindowInfo? {
    listOnscreenWindows().first { $0.bounds.contains(point) }
}

func windowCGOnscreenBounds(windowNumber: Int) -> CGRect? {
    guard windowNumber > 0 else {
        return nil
    }
    let options: CGWindowListOption = [.optionOnScreenOnly, .excludeDesktopElements]
    guard let raw = CGWindowListCopyWindowInfo(options, kCGNullWindowID) as? [[String: Any]] else {
        return nil
    }

    return raw.first(where: { ($0[kCGWindowNumber as String] as? Int) == windowNumber })
        .flatMap { entry in
            (entry[kCGWindowBounds as String] as? NSDictionary).flatMap(CGRect.init(dictionaryRepresentation:))
        }
}

func windowIsOnscreen(windowNumber: Int) -> Bool {
    guard windowNumber > 0 else {
        return true
    }
    let options: CGWindowListOption = [.optionOnScreenOnly, .excludeDesktopElements]
    guard let raw = CGWindowListCopyWindowInfo(options, kCGNullWindowID) as? [[String: Any]] else {
        return true
    }
    return raw.contains(where: { ($0[kCGWindowNumber as String] as? Int) == windowNumber && ($0[kCGWindowLayer as String] as? Int) == 0 })
}

private func resolveAXWindow(info: WindowInfo) -> AXUIElement? {
    let appElement = AXUIElementCreateApplication(info.pid)
    var value: CFTypeRef?
    guard AXUIElementCopyAttributeValue(appElement, kAXWindowsAttribute as CFString, &value) == .success,
          let windows = value as? [AXUIElement]
    else {
        return nil
    }

    return windows.min { lhs, rhs in
        let lhsDistance = distance(from: axWindowFrame(lhs), to: info.bounds)
        let rhsDistance = distance(from: axWindowFrame(rhs), to: info.bounds)
        return lhsDistance < rhsDistance
    }
}

private func distance(from lhs: CGRect?, to rhs: CGRect) -> CGFloat {
    guard let lhs else {
        return .greatestFiniteMagnitude
    }
    return abs(lhs.minX - rhs.minX) + abs(lhs.minY - rhs.minY) + abs(lhs.width - rhs.width) + abs(lhs.height - rhs.height)
}

private func visibleFrameQuartz(at point: CGPoint) -> CGRect {
    for screen in NSScreen.screens {
        let visible = screen.visibleFrame
        let quartz = CGRect(x: visible.minX, y: primaryScreenHeight() - visible.minY - visible.height, width: visible.width, height: visible.height)
        if quartz.contains(point) {
            return quartz
        }
    }
    let fallback = NSScreen.main?.visibleFrame ?? .zero
    return CGRect(x: fallback.minX, y: primaryScreenHeight() - fallback.minY - fallback.height, width: fallback.width, height: fallback.height)
}

private func frameNearVisibleMaximize(_ frame: CGRect) -> Bool {
    let visible = visibleFrameQuartz(at: frame.center)
    return abs(frame.minX - visible.minX) <= frameTolerance &&
        abs(frame.minY - visible.minY) <= frameTolerance &&
        abs(frame.width - visible.width) <= frameTolerance &&
        abs(frame.height - visible.height) <= frameTolerance
}

@MainActor
final class WindowSnapController {
    private var snappedWindow: AXUIElement?
    private(set) var snappedReference: String?
    private var snappedNumber = 0
    private var snappedPID: pid_t = 0

    func clear() {
        snappedWindow = nil
        snappedReference = nil
        snappedNumber = 0
        snappedPID = 0
    }

    func pick(from info: WindowInfo) -> (String, CGRect) {
        snappedWindow = resolveAXWindow(info: info)
        snappedReference = info.name.isEmpty ? "pid \(info.pid)" : info.name
        snappedNumber = info.windowNumber
        snappedPID = info.pid
        return (snappedReference ?? "Window", info.bounds)
    }

    func restoreWindow() -> CGRect? {
        guard let snappedWindow else {
            return nil
        }

        AXUIElementSetAttributeValue(snappedWindow, kAXMinimizedAttribute as CFString, kCFBooleanFalse)
        AXUIElementPerformAction(snappedWindow, kAXRaiseAction as CFString)

        if let app = NSRunningApplication(processIdentifier: snappedPID) {
            app.activate(options: NSApplication.ActivationOptions.activateAllWindows)
        }

        return awaitStableFrame()
    }

    func currentWindowFrame() -> CGRect? {
        guard let snappedWindow else {
            return nil
        }
        return axWindowFrame(snappedWindow)
    }

    func snapRegionToWindow(_ region: inout AbsoluteConfig, windowFrame: CGRect) {
        let targetHeight = windowFrame.width / RM2.aspect
        let targetWidth = targetHeight > windowFrame.height ? windowFrame.height * RM2.aspect : windowFrame.width
        region.regionX = windowFrame.minX
        region.regionY = windowFrame.minY
        region.regionWidth = targetWidth
        region.lockAspect()
    }

    func syncRegionToWindow(_ region: inout AbsoluteConfig) -> Bool {
        guard let frame = currentWindowFrame() else {
            return false
        }
        snapRegionToWindow(&region, windowFrame: frame)
        return true
    }

    func syncWindowToRegion(_ region: AbsoluteConfig) {
        guard let snappedWindow else {
            return
        }
        var point = CGPoint(x: region.regionX, y: region.regionY)
        var size = CGSize(width: region.regionWidth, height: region.regionHeight)
        guard let pointValue = AXValueCreate(.cgPoint, &point),
              let sizeValue = AXValueCreate(.cgSize, &size)
        else {
            return
        }

        AXUIElementSetAttributeValue(snappedWindow, kAXSizeAttribute as CFString, sizeValue)
        AXUIElementSetAttributeValue(snappedWindow, kAXPositionAttribute as CFString, pointValue)
        AXUIElementSetAttributeValue(snappedWindow, kAXSizeAttribute as CFString, sizeValue)
    }

    func snappedLifecycleState() -> WindowLifecycle {
        guard let snappedWindow else {
            return .closed
        }

        if !elementAlive(snappedWindow) {
            return .closed
        }

        if axBool(axCopyAttribute(snappedWindow, kAXMinimizedAttribute as CFString)) == true {
            return .minimized
        }
        if !windowIsOnscreen(windowNumber: snappedNumber) {
            return .minimized
        }
        if isStageManagerMinimized() {
            return .minimized
        }

        guard let frame = axWindowFrame(snappedWindow) else {
            return .normal
        }
        if axBool(axCopyAttribute(snappedWindow, "AXFullScreen" as CFString)) == true || frameNearVisibleMaximize(frame) {
            return .maximized
        }

        return .normal
    }

    private func isStageManagerMinimized() -> Bool {
        guard let snappedWindow,
              let axFrame = axWindowFrame(snappedWindow),
              let cgBounds = windowCGOnscreenBounds(windowNumber: snappedNumber)
        else {
            return false
        }

        let axArea = axFrame.width * axFrame.height
        guard axArea > 0 else {
            return false
        }

        return (cgBounds.width * cgBounds.height) / axArea < stageAreaRatio
    }

    private func elementAlive(_ element: AXUIElement) -> Bool {
        var value: CFTypeRef?
        let error = AXUIElementCopyAttributeValue(element, kAXRoleAttribute as CFString, &value)
        return error != .invalidUIElement
    }

    private func awaitStableFrame() -> CGRect? {
        var previous: CGRect?
        for _ in 0..<10 {
            let current = currentWindowFrame()
            if let current, let previous,
               abs(current.width - previous.width) < 2,
               abs(current.height - previous.height) < 2 {
                return current
            }
            previous = current
            Thread.sleep(forTimeInterval: 0.05)
        }
        return previous
    }
}
