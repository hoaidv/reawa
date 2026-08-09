// Traceability (ADLC iter-000)
// @implements SRS-RW-37
// @implements SRS-RW-39
// @implements SRS-RW-40
// @implements SRS-RW-44
// @implements SRS-RW-45
// @implements SRS-RW-48

import AppKit
import CoreGraphics
import Foundation

private let handleSize: CGFloat = 18
private let minimumRegionWidth: CGFloat = 160

private final class PickerWindow: NSWindow {
    override var canBecomeKey: Bool { true }

    override func constrainFrameRect(_ frameRect: NSRect, to screen: NSScreen?) -> NSRect {
        frameRect
    }
}

private final class SnapPickerView: NSView {
    var highlight: CGRect?
    var showHint = true
    var onPick: ((WindowInfo) -> Void)?
    var onCancel: (() -> Void)?
    var windowOrigin = CGPoint.zero

    override var acceptsFirstResponder: Bool { true }
    override var isOpaque: Bool { false }

    override func draw(_ dirtyRect: NSRect) {
        NSColor.black.withAlphaComponent(0.28).setFill()
        dirtyRect.fill()

        if showHint {
            let attributes: [NSAttributedString.Key: Any] = [
                .foregroundColor: NSColor.white,
                .font: NSFont.boldSystemFont(ofSize: 15),
            ]
            let message = "Click a window to snap the reMarkable region  ·  Esc to cancel"
            let size = message.size(withAttributes: attributes)
            let point = CGPoint(x: bounds.midX - size.width / 2, y: bounds.maxY - 64)
            message.draw(at: point, withAttributes: attributes)
        }

        if let highlight {
            NSColor(calibratedRed: 0.23, green: 0.51, blue: 0.96, alpha: 0.22).setFill()
            highlight.fill()
            NSColor(calibratedRed: 0.23, green: 0.51, blue: 0.96, alpha: 1).setStroke()
            let border = NSBezierPath(rect: highlight)
            border.lineWidth = 3
            border.stroke()
        }
    }

    override func mouseDown(with event: NSEvent) {
        let point = CGEvent(source: nil)?.location ?? .zero
        if let info = windowUnderPoint(point) {
            onPick?(info)
        }
    }

    override func keyDown(with event: NSEvent) {
        if event.keyCode == 53 {
            onCancel?()
        }
    }
}

@MainActor
final class PickerOverlayController {
    private var windows: [PickerWindow] = []
    private var views: [SnapPickerView] = []
    private var timer: Timer?
    private var onPick: ((WindowInfo) -> Void)?
    private var onCancel: (() -> Void)?
    private var onHoverChange: ((WindowInfo?) -> Void)?
    private var lastHoveredWindow: WindowInfo?

    func start(
        onHoverChange: ((WindowInfo?) -> Void)? = nil,
        onPick: @escaping (WindowInfo) -> Void,
        onCancel: @escaping () -> Void
    ) {
        stop()
        self.onPick = onPick
        self.onCancel = onCancel
        self.onHoverChange = onHoverChange

        var showHint = true
        for screen in NSScreen.screens {
            let window = PickerWindow(
                contentRect: screen.frame,
                styleMask: .borderless,
                backing: .buffered,
                defer: false
            )
            window.level = .screenSaver
            window.isOpaque = false
            window.backgroundColor = .clear
            window.ignoresMouseEvents = false
            window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]

            let view = SnapPickerView(frame: CGRect(origin: .zero, size: screen.frame.size))
            view.windowOrigin = screen.frame.origin
            view.showHint = showHint
            view.onPick = { [weak self] info in
                self?.handlePick(info)
            }
            view.onCancel = { [weak self] in
                self?.handleCancel()
            }
            window.contentView = view

            windows.append(window)
            views.append(view)
            showHint = false
        }

        NSApp.activate(ignoringOtherApps: true)
        for window in windows {
            window.orderFrontRegardless()
        }
        if let first = windows.first, let firstView = views.first {
            first.makeKeyAndOrderFront(nil)
            first.makeFirstResponder(firstView)
        }

        timer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
            Task { @MainActor in
                self?.poll()
            }
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        if lastHoveredWindow != nil {
            onHoverChange?(nil)
        }
        for window in windows {
            window.orderOut(nil)
        }
        windows.removeAll()
        views.removeAll()
        lastHoveredWindow = nil
        onHoverChange = nil
    }

    private func poll() {
        guard !views.isEmpty else {
            return
        }
        let point = CGEvent(source: nil)?.location ?? .zero
        guard let info = windowUnderPoint(point) else {
            if lastHoveredWindow != nil {
                lastHoveredWindow = nil
                onHoverChange?(nil)
            }
            for view in views where view.highlight != nil {
                view.highlight = nil
                view.needsDisplay = true
            }
            return
        }

        if info != lastHoveredWindow {
            lastHoveredWindow = info
            onHoverChange?(info)
        }

        let cocoaRect = cgRectToCocoa(info.bounds)
        for view in views {
            let local = cocoaRect.offsetBy(dx: -view.windowOrigin.x, dy: -view.windowOrigin.y)
            if view.highlight != local {
                view.highlight = local
                view.needsDisplay = true
            }
        }
    }

    private func handlePick(_ info: WindowInfo) {
        let callback = onPick
        stop()
        callback?(info)
    }

    private func handleCancel() {
        let callback = onCancel
        stop()
        callback?()
    }
}

private final class OverlayWindow: NSWindow {
    override func constrainFrameRect(_ frameRect: NSRect, to screen: NSScreen?) -> NSRect {
        frameRect
    }
}

private final class RegionOverlayView: NSView {
    var highlightRect: CGRect?
    var borderColor = AbsoluteConfig.defaultBorderColor
    var borderStyle = "solid"

    override var isOpaque: Bool { false }

    override func draw(_ dirtyRect: NSRect) {
        guard let highlightRect else {
            return
        }

        let components = hexColorComponents(borderColor)
        NSColor(calibratedRed: components.0, green: components.1, blue: components.2, alpha: 1).setStroke()
        let path = NSBezierPath(rect: highlightRect)
        path.lineWidth = 3
        if borderStyle == "dashed" {
            path.setLineDash([8, 6], count: 2, phase: 0)
        }
        path.stroke()
    }
}

private final class RegionHandleWindow: NSWindow {
    override var canBecomeKey: Bool { true }
}

private final class RegionHandleView: NSView {
    let corner: String
    weak var controller: RegionOverlayController?

    init(corner: String, controller: RegionOverlayController) {
        self.corner = corner
        self.controller = controller
        super.init(frame: CGRect(origin: .zero, size: CGSize(width: handleSize, height: handleSize)))
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func resetCursorRects() {
        addCursorRect(bounds, cursor: .crosshair)
    }

    override func draw(_ dirtyRect: NSRect) {
        NSColor(calibratedRed: 0.23, green: 0.51, blue: 0.96, alpha: 1).setFill()
        let dotRect = CGRect(x: bounds.midX - 5, y: bounds.midY - 5, width: 10, height: 10)
        let path = NSBezierPath(ovalIn: dotRect)
        path.fill()
        NSColor.white.setStroke()
        path.lineWidth = 1.5
        path.stroke()
    }

    override func mouseDragged(with event: NSEvent) {
        let point = CGEvent(source: nil)?.location ?? .zero
        controller?.resizeFromCorner(corner, pointer: point)
    }
}

@MainActor
final class RegionOverlayController {
    private let onRegionChanged: (AbsoluteConfig) -> Void
    private var region = AbsoluteConfig()
    private var window: OverlayWindow?
    private var view: RegionOverlayView?
    private var origin = CGPoint.zero
    private var handleWindows: [String: RegionHandleWindow] = [:]

    init(onRegionChanged: @escaping (AbsoluteConfig) -> Void) {
        self.onRegionChanged = onRegionChanged
    }

    func show(_ region: AbsoluteConfig) {
        self.region = region
        let frame = desktopBoundsCocoa()
        origin = frame.origin

        if window == nil {
            let overlay = OverlayWindow(contentRect: frame, styleMask: .borderless, backing: .buffered, defer: false)
            overlay.level = .floating
            overlay.isOpaque = false
            overlay.backgroundColor = .clear
            overlay.ignoresMouseEvents = true
            overlay.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
            let content = RegionOverlayView(frame: CGRect(origin: .zero, size: frame.size))
            overlay.contentView = content
            window = overlay
            view = content
        } else {
            window?.setFrame(frame, display: false)
            view?.frame = CGRect(origin: .zero, size: frame.size)
        }

        buildHandles()
        refresh()
        window?.orderFrontRegardless()
    }

    func hide() {
        window?.orderOut(nil)
        for handle in handleWindows.values {
            handle.orderOut(nil)
        }
    }

    func updateRegion(_ region: AbsoluteConfig) {
        self.region = region
        refresh()
    }

    func resizeFromCorner(_ corner: String, pointer: CGPoint) {
        var next = region
        let x = region.regionX
        let y = region.regionY
        let width = region.regionWidth
        let height = region.regionHeight
        let anchor: CGPoint

        switch corner {
        case "tl":
            anchor = CGPoint(x: x + width, y: y + height)
        case "tr":
            anchor = CGPoint(x: x, y: y + height)
        case "bl":
            anchor = CGPoint(x: x + width, y: y)
        default:
            anchor = CGPoint(x: x, y: y)
        }

        let nextWidth = max(minimumRegionWidth, abs(anchor.x - pointer.x))
        let nextHeight = nextWidth / RM2.aspect
        next.regionWidth = nextWidth
        next.lockAspect()
        next.regionX = pointer.x < anchor.x ? anchor.x - nextWidth : anchor.x
        next.regionY = pointer.y < anchor.y ? anchor.y - nextHeight : anchor.y

        region = next
        refresh()
        onRegionChanged(next)
    }

    private func refresh() {
        guard let view else {
            return
        }
        let localRect = cgRectToCocoa(region.rect).offsetBy(dx: -origin.x, dy: -origin.y)
        view.highlightRect = localRect
        view.borderColor = region.borderColor
        view.borderStyle = region.borderStyle
        view.needsDisplay = true
        positionHandles()
    }

    private func buildHandles() {
        guard handleWindows.isEmpty else {
            return
        }

        for corner in ["tl", "tr", "bl", "br"] {
            let handleWindow = RegionHandleWindow(
                contentRect: CGRect(x: 0, y: 0, width: handleSize, height: handleSize),
                styleMask: .borderless,
                backing: .buffered,
                defer: false
            )
            handleWindow.level = .screenSaver
            handleWindow.isOpaque = false
            handleWindow.backgroundColor = .clear
            handleWindow.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
            handleWindow.contentView = RegionHandleView(corner: corner, controller: self)
            handleWindows[corner] = handleWindow
        }
    }

    private func positionHandles() {
        let corners: [String: CGPoint] = [
            "tl": CGPoint(x: region.regionX, y: region.regionY),
            "tr": CGPoint(x: region.regionX + region.regionWidth, y: region.regionY),
            "bl": CGPoint(x: region.regionX, y: region.regionY + region.regionHeight),
            "br": CGPoint(x: region.regionX + region.regionWidth, y: region.regionY + region.regionHeight),
        ]

        for (corner, point) in corners {
            guard let window = handleWindows[corner] else {
                continue
            }
            let cocoa = cgPointToCocoa(point)
            let frame = CGRect(x: cocoa.x - handleSize / 2, y: cocoa.y - handleSize / 2, width: handleSize, height: handleSize)
            window.setFrame(frame, display: true)
            window.orderFrontRegardless()
        }
    }
}
