import XCTest
@testable import ReawaApp

final class LoggingTests: XCTestCase {
    func testBehaviorLogEntryIncludesCategoryAndMessage() {
        let entry = LogEntry(level: "info", category: .settings, message: "tablet orientation -> Gut to the left")

        XCTAssertTrue(entry.formatted.contains("[settings]"))
        XCTAssertTrue(entry.formatted.contains("tablet orientation -> Gut to the left"))
        XCTAssertTrue(entry.searchableText.contains("info"))
    }

    func testPenLogEntryFormatsRequestedColumns() {
        let entry = PenLogEntry(
            rawData: "EV_KEY BTN_TOUCH 1",
            semantic: "PEN TOUCH (x, y) = (73, 110)",
            gestureState: .start
        )

        XCTAssertTrue(entry.formatted.contains("EV_KEY BTN_TOUCH 1 | PEN TOUCH (x, y) = (73, 110) | START"))
        XCTAssertTrue(entry.searchableText.contains("BTN_TOUCH"))
    }
}
