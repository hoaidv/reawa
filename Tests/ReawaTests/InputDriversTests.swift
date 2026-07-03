import CoreGraphics
import XCTest
@testable import ReawaApp

final class InputDriversTests: XCTestCase {
    func testFallbackDisplayIDReturnsMatchingActiveDisplay() {
        let displays = [
            DisplayCandidate(id: 1, bounds: CGRect(x: 0, y: 0, width: 1440, height: 900)),
            DisplayCandidate(id: 2, bounds: CGRect(x: 1440, y: 0, width: 2560, height: 1440)),
        ]

        let displayID = MouseController.fallbackDisplayID(
            at: CGPoint(x: 2000, y: 400),
            activeDisplays: displays,
            defaultDisplayID: 1
        )

        XCTAssertEqual(displayID, 2)
    }

    func testFallbackDisplayIDUsesDefaultWhenNoDisplayContainsPoint() {
        let displays = [
            DisplayCandidate(id: 11, bounds: CGRect(x: 0, y: 0, width: 1440, height: 900)),
            DisplayCandidate(id: 22, bounds: CGRect(x: 1440, y: 0, width: 2560, height: 1440)),
        ]

        let displayID = MouseController.fallbackDisplayID(
            at: CGPoint(x: -100, y: -100),
            activeDisplays: displays,
            defaultDisplayID: 99
        )

        XCTAssertEqual(displayID, 99)
    }

    func testResolvedScalePrefersConfiguredScale() {
        let scale = MouseController.resolvedScale(configuredScale: 1.25, displayPPI: 220)
        XCTAssertEqual(scale, 1.25, accuracy: 0.0001)
    }

    func testResolvedScaleUsesDisplayPPIWhenConfiguredScaleIsNil() {
        let scale = MouseController.resolvedScale(configuredScale: nil, displayPPI: 132.8)
        XCTAssertEqual(scale, 132.8 / RM2.dpi, accuracy: 0.0001)
    }

    func testResolvedScaleFallsBackToOneWhenPPIUnavailable() {
        let scale = MouseController.resolvedScale(configuredScale: nil, displayPPI: nil)
        XCTAssertEqual(scale, 1.0, accuracy: 0.0001)
    }

    func testRelativeGestureMapsCursorFromAnchorState() {
        let mouse = MouseController(
            config: DeviceConfig(
                outputMode: .relative,
                scale: 1.0,
                swapXY: false,
                invertX: false,
                invertY: false,
                absolute: AbsoluteConfig()
            )
        )
        let gesture = RelativeGesture(
            phase: .hover,
            anchorPenX: 100,
            anchorPenY: 200,
            anchorCursor: CGPoint(x: 500, y: 600),
            lastPenX: 100,
            lastPenY: 200
        )

        let cursor = gesture.cursor(mouse: mouse, penX: 130, penY: 180)
        XCTAssertEqual(cursor.x, 530, accuracy: 0.0001)
        XCTAssertEqual(cursor.y, 580, accuracy: 0.0001)
    }

    func testRelativeGestureRebaseUsesLastPenPointAsNewAnchor() {
        var gesture = RelativeGesture(
            phase: .hover,
            anchorPenX: 100,
            anchorPenY: 200,
            anchorCursor: CGPoint(x: 500, y: 600),
            lastPenX: 140,
            lastPenY: 260
        )

        gesture.rebase(to: CGPoint(x: 700, y: 800))

        XCTAssertEqual(gesture.anchorPenX, 140)
        XCTAssertEqual(gesture.anchorPenY, 260)
        XCTAssertEqual(gesture.anchorCursor.x, 700, accuracy: 0.0001)
        XCTAssertEqual(gesture.anchorCursor.y, 800, accuracy: 0.0001)
    }

    func testRelativeDriverRebasesWhenLiveCursorMovesAwayFromExpected() {
        XCTAssertTrue(
            RelativePenDriver.shouldRebaseGesture(
                liveCursor: CGPoint(x: 160, y: 100),
                expectedCursor: CGPoint(x: 100, y: 100)
            )
        )
    }

    func testRelativeDriverDoesNotRebaseForTinyCursorDifferences() {
        XCTAssertFalse(
            RelativePenDriver.shouldRebaseGesture(
                liveCursor: CGPoint(x: 103, y: 104),
                expectedCursor: CGPoint(x: 100, y: 100)
            )
        )
    }
}
