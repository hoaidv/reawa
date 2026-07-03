import ApplicationServices
import Foundation

struct DisplayCandidate: Sendable {
    let id: CGDirectDisplayID
    let bounds: CGRect
}

final class MouseController {
    var config: DeviceConfig

    init(config: DeviceConfig) {
        self.config = config
    }

    func currentCursor() -> CGPoint {
        CGEvent(source: nil)?.location ?? .zero
    }

    func desktopBounds() -> CGRect {
        let maxDisplays: UInt32 = 16
        var activeCount: UInt32 = 0
        var activeDisplays = Array(repeating: CGDirectDisplayID(), count: Int(maxDisplays))
        let error = CGGetActiveDisplayList(maxDisplays, &activeDisplays, &activeCount)
        guard error == .success, activeCount > 0 else {
            return CGDisplayBounds(CGMainDisplayID())
        }

        let bounds = activeDisplays.prefix(Int(activeCount)).map(CGDisplayBounds)
        return bounds.dropFirst().reduce(bounds[0]) { $0.union($1) }
    }

    func clamp(_ point: CGPoint) -> CGPoint {
        let bounds = desktopBounds()
        return CGPoint(
            x: clampValue(point.x, min: bounds.minX, max: bounds.maxX - 1),
            y: clampValue(point.y, min: bounds.minY, max: bounds.maxY - 1)
        )
    }

    func clamp(_ point: CGPoint, to rect: CGRect) -> CGPoint {
        CGPoint(
            x: clampValue(point.x, min: rect.minX, max: rect.maxX - 1),
            y: clampValue(point.y, min: rect.minY, max: rect.maxY - 1)
        )
    }

    func displayID(at point: CGPoint) -> CGDirectDisplayID {
        let maxDisplays: UInt32 = 16
        var matching = Array(repeating: CGDirectDisplayID(), count: Int(maxDisplays))
        var count: UInt32 = 0
        let error = CGGetDisplaysWithPoint(point, maxDisplays, &matching, &count)
        if error == .success, count > 0 {
            return matching[0]
        }
        var activeDisplays = Array(repeating: CGDirectDisplayID(), count: Int(maxDisplays))
        var activeCount: UInt32 = 0
        let activeError = CGGetActiveDisplayList(maxDisplays, &activeDisplays, &activeCount)
        if activeError == .success, activeCount > 0 {
            return Self.fallbackDisplayID(
                at: point,
                activeDisplays: activeDisplays.prefix(Int(activeCount)).map {
                    DisplayCandidate(id: $0, bounds: CGDisplayBounds($0))
                },
                defaultDisplayID: CGMainDisplayID()
            )
        }
        return CGMainDisplayID()
    }

    func displayLogicalPPI(_ displayID: CGDirectDisplayID) -> Double? {
        let bounds = CGDisplayBounds(displayID)
        let sizeMM = CGDisplayScreenSize(displayID)
        guard sizeMM.width > 0 else {
            return nil
        }
        return Double(bounds.width) / (Double(sizeMM.width) / 25.4)
    }

    func effectiveScale(at point: CGPoint) -> Double {
        let displayID = displayID(at: point)
        let displayPPI = displayLogicalPPI(displayID)
        return Self.resolvedScale(
            configuredScale: config.scale,
            displayPPI: displayPPI
        )
    }

    func mapDelta(dx: Int, dy: Int, scale: Double) -> CGPoint {
        var mappedX = dx
        var mappedY = dy
        if config.swapXY {
            swap(&mappedX, &mappedY)
        }
        if config.invertX {
            mappedX *= -1
        }
        if config.invertY {
            mappedY *= -1
        }
        return CGPoint(x: Double(mappedX) * scale, y: Double(mappedY) * scale)
    }

    func mapPenCoordinates(x: Int, y: Int) -> CGPoint {
        var mappedX = x
        var mappedY = y
        if config.swapXY {
            swap(&mappedX, &mappedY)
        }
        if config.invertX {
            mappedX = RM2.penXMax - mappedX
        }
        if config.invertY {
            mappedY = RM2.penYMax - mappedY
        }
        return CGPoint(x: mappedX, y: mappedY)
    }

    func postMouseEvent(type: CGEventType, at point: CGPoint) {
        guard let event = CGEvent(mouseEventSource: nil, mouseType: type, mouseCursorPosition: point, mouseButton: .left) else {
            return
        }
        event.post(tap: .cghidEventTap)
    }

    func releaseButton(at point: CGPoint) {
        postMouseEvent(type: .leftMouseUp, at: point)
    }

    static func fallbackDisplayID(
        at point: CGPoint,
        activeDisplays: [DisplayCandidate],
        defaultDisplayID: CGDirectDisplayID
    ) -> CGDirectDisplayID {
        for display in activeDisplays where display.bounds.contains(point) {
            return display.id
        }
        return defaultDisplayID
    }

    static func resolvedScale(configuredScale: Double?, displayPPI: Double?) -> Double {
        if let configuredScale {
            return configuredScale
        }
        guard let displayPPI else {
            return 1.0
        }
        return displayPPI / RM2.dpi
    }
}

protocol PenDriver: AnyObject {
    func handle(frame: PenFrame)
    func cleanup()
}

enum RelativeGesturePhase {
    case hover
    case touch
}

struct RelativeGesture {
    var phase: RelativeGesturePhase
    var anchorPenX: Int
    var anchorPenY: Int
    var anchorCursor: CGPoint
    var lastPenX: Int
    var lastPenY: Int

    func cursor(mouse: MouseController, penX: Int, penY: Int) -> CGPoint {
        let scale = mouse.effectiveScale(at: anchorCursor)
        let delta = mouse.mapDelta(dx: penX - anchorPenX, dy: penY - anchorPenY, scale: scale)
        guard delta.x != 0 || delta.y != 0 else {
            return anchorCursor
        }
        return mouse.clamp(CGPoint(x: anchorCursor.x + delta.x, y: anchorCursor.y + delta.y))
    }

    func expectedCursor(mouse: MouseController) -> CGPoint {
        cursor(mouse: mouse, penX: lastPenX, penY: lastPenY)
    }

    mutating func rebase(to liveCursor: CGPoint) {
        anchorCursor = liveCursor
        anchorPenX = lastPenX
        anchorPenY = lastPenY
    }

    mutating func advance(to penX: Int, _ penY: Int) {
        lastPenX = penX
        lastPenY = penY
    }
}

final class RelativePenDriver: PenDriver {
    private static let externalCursorRebaseDistanceThreshold: CGFloat = 6

    private let mouse: MouseController
    private var buttonDown = false
    private var gesture: RelativeGesture?

    init(mouse: MouseController) {
        self.mouse = mouse
    }

    func handle(frame: PenFrame) {
        let liveCursor = mouse.currentCursor()
        guard frame.inProximity else {
            let cursor = gesture?.cursor(mouse: mouse, penX: frame.x, penY: frame.y) ?? liveCursor
            if buttonDown {
                mouse.releaseButton(at: cursor)
                buttonDown = false
            }
            gesture = nil
            return
        }

        let phase: RelativeGesturePhase = frame.touching ? .touch : .hover

        guard var gesture else {
            self.gesture = RelativeGesture(
                phase: phase,
                anchorPenX: frame.x,
                anchorPenY: frame.y,
                anchorCursor: liveCursor,
                lastPenX: frame.x,
                lastPenY: frame.y
            )
            if phase == .touch && !buttonDown {
                mouse.postMouseEvent(type: .leftMouseDown, at: liveCursor)
                buttonDown = true
            }
            return
        }

        let previousCursor = gesture.expectedCursor(mouse: mouse)
        if Self.shouldRebaseGesture(liveCursor: liveCursor, expectedCursor: previousCursor) {
            gesture.rebase(to: liveCursor)
        }

        let cursor = gesture.cursor(mouse: mouse, penX: frame.x, penY: frame.y)

        if gesture.phase != phase {
            if phase == .touch {
                if !buttonDown {
                    mouse.postMouseEvent(type: .leftMouseDown, at: cursor)
                    buttonDown = true
                }
            } else if buttonDown {
                mouse.postMouseEvent(type: .leftMouseUp, at: cursor)
                buttonDown = false
            }

            self.gesture = RelativeGesture(
                phase: phase,
                anchorPenX: frame.x,
                anchorPenY: frame.y,
                anchorCursor: cursor,
                lastPenX: frame.x,
                lastPenY: frame.y
            )
            return
        }

        if cursor != previousCursor {
            mouse.postMouseEvent(type: phase == .touch ? .leftMouseDragged : .mouseMoved, at: cursor)
        }

        gesture.advance(to: frame.x, frame.y)
        self.gesture = gesture
    }

    func cleanup() {
        if buttonDown {
            let cursor = gesture?.expectedCursor(mouse: mouse) ?? mouse.currentCursor()
            mouse.releaseButton(at: cursor)
            buttonDown = false
        }
        gesture = nil
    }

    static func shouldRebaseGesture(
        liveCursor: CGPoint,
        expectedCursor: CGPoint
    ) -> Bool {
        hypot(liveCursor.x - expectedCursor.x, liveCursor.y - expectedCursor.y) >= externalCursorRebaseDistanceThreshold
    }
}

final class AbsolutePenDriver: PenDriver {
    private let mouse: MouseController
    private var region: AbsoluteConfig
    private var wasTouching = false
    private var buttonDown = false

    init(mouse: MouseController, region: AbsoluteConfig) {
        self.mouse = mouse
        self.region = region
    }

    func updateRegion(_ region: AbsoluteConfig) {
        self.region = region
    }

    func handle(frame: PenFrame) {
        guard frame.inProximity else {
            if buttonDown {
                mouse.releaseButton(at: mouse.currentCursor())
                buttonDown = false
            }
            wasTouching = false
            return
        }

        let mapped = mouse.mapPenCoordinates(x: frame.x, y: frame.y)
        let rect = region.rect
        let point = mouse.clamp(
            CGPoint(
                x: rect.minX + (mapped.x / CGFloat(RM2.penXMax)) * rect.width,
                y: rect.minY + (mapped.y / CGFloat(RM2.penYMax)) * rect.height
            ),
            to: rect
        )

        if frame.touching && !wasTouching {
            mouse.postMouseEvent(type: .leftMouseDown, at: point)
            buttonDown = true
        } else if !frame.touching && wasTouching {
            mouse.postMouseEvent(type: .leftMouseUp, at: point)
            buttonDown = false
        } else {
            mouse.postMouseEvent(type: frame.touching ? .leftMouseDragged : .mouseMoved, at: point)
        }

        wasTouching = frame.touching
    }

    func cleanup() {
        if buttonDown {
            mouse.releaseButton(at: mouse.currentCursor())
            buttonDown = false
        }
    }
}
