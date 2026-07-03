import Foundation

enum BehaviorLogCategory: String, CaseIterable, Sendable {
    case app
    case settings
    case connection
    case session
    case ssh
    case mode
    case absolute
    case discovery
    case notification

    var title: String {
        switch self {
        case .app:
            return "app"
        case .settings:
            return "settings"
        case .connection:
            return "connection"
        case .session:
            return "session"
        case .ssh:
            return "ssh"
        case .mode:
            return "mode"
        case .absolute:
            return "absolute"
        case .discovery:
            return "discovery"
        case .notification:
            return "notification"
        }
    }
}

enum PenGestureState: String, Sendable {
    case start = "START"
    case move = "MOVE"
    case end = "END"
    case out = "OUT"
}

private extension Date {
    var logTimestamp: String {
        LogDateFormatter.shared.string(from: self)
    }
}

private enum LogDateFormatter {
    static let shared: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss.SSS"
        return formatter
    }()
}

private let behaviorLogMaxEntries = 2_000
private let penLogMaxEntries = 8_000

struct LogEntry: Identifiable, Equatable, Sendable {
    let id = UUID()
    let timestamp = Date()
    let level: String
    let category: BehaviorLogCategory
    let message: String

    var timestampText: String {
        timestamp.logTimestamp
    }

    var formatted: String {
        "\(timestampText)  [\(category.title)]  \(message)"
    }

    var searchableText: String {
        "\(formatted) \(level)"
    }
}

struct PenLogEntry: Identifiable, Equatable, Sendable {
    let id = UUID()
    let timestamp = Date()
    let rawData: String
    let semantic: String
    let gestureState: PenGestureState?

    var timestampText: String {
        timestamp.logTimestamp
    }

    var formatted: String {
        let gesture = gestureState?.rawValue ?? "-"
        return "\(timestampText)  \(rawData) | \(semantic) | \(gesture)"
    }

    var searchableText: String {
        "\(rawData) \(semantic) \(gestureState?.rawValue ?? "") \(timestamp.logTimestamp)"
    }
}

private struct LoggerSnapshot {
    let behaviorEntries: [LogEntry]
    let penEntries: [PenLogEntry]
    let penCapabilityLabels: [String]
    let penLoggingEnabled: Bool
    let penSessionLabel: String?
}

private final class LoggerState: @unchecked Sendable {
    private var behaviorEntries: [LogEntry] = []
    private var penEntries: [PenLogEntry] = []
    private var penCapabilityLabels: Set<String> = []
    private var penLoggingEnabled = false
    private var penSessionLabel: String?
    private var publishScheduled = false
    private let lock = NSLock()

    func appendBehavior(_ entry: LogEntry, maxEntries: Int) {
        lock.lock()
        behaviorEntries.append(entry)
        if behaviorEntries.count > maxEntries {
            behaviorEntries.removeFirst(behaviorEntries.count - maxEntries)
        }
        lock.unlock()
    }

    @discardableResult
    func appendPen(_ entry: PenLogEntry, capabilities: [String], maxEntries: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }
        guard penLoggingEnabled else {
            return false
        }
        penEntries.append(entry)
        if penEntries.count > maxEntries {
            penEntries.removeFirst(penEntries.count - maxEntries)
        }
        penCapabilityLabels.formUnion(capabilities)
        return true
    }

    func setPenLoggingEnabled(_ enabled: Bool) {
        lock.lock()
        penLoggingEnabled = enabled
        lock.unlock()
    }

    func clearBehavior() {
        lock.lock()
        behaviorEntries.removeAll()
        lock.unlock()
    }

    func clearPen() {
        lock.lock()
        penEntries.removeAll()
        penCapabilityLabels.removeAll()
        lock.unlock()
    }

    func beginPenSession(label: String) {
        lock.lock()
        penSessionLabel = label
        penEntries.removeAll()
        penCapabilityLabels.removeAll()
        lock.unlock()
    }

    func setPenSessionLabel(_ label: String?) {
        lock.lock()
        penSessionLabel = label
        lock.unlock()
    }

    func markPublishScheduled() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        if publishScheduled {
            return false
        }
        publishScheduled = true
        return true
    }

    func snapshot() -> LoggerSnapshot {
        lock.lock()
        defer {
            publishScheduled = false
            lock.unlock()
        }
        return LoggerSnapshot(
            behaviorEntries: behaviorEntries,
            penEntries: penEntries,
            penCapabilityLabels: penCapabilityLabels.sorted(),
            penLoggingEnabled: penLoggingEnabled,
            penSessionLabel: penSessionLabel
        )
    }
}

@MainActor
final class AppLogger: ObservableObject {
    static let shared = AppLogger()

    @Published private(set) var behaviorEntries: [LogEntry] = []
    @Published private(set) var penEntries: [PenLogEntry] = []
    @Published private(set) var penCapabilityLabels: [String] = []
    @Published private(set) var penLoggingEnabled = false
    @Published private(set) var penSessionLabel: String?

    private let state = LoggerState()

    func log(
        _ message: String,
        level: String = "log",
        category: BehaviorLogCategory = .app
    ) {
        logBehavior(message, level: level, category: category)
    }

    func setPenLoggingEnabled(_ enabled: Bool) {
        state.setPenLoggingEnabled(enabled)
        log(
            enabled ? "Pen event capture enabled." : "Pen event capture disabled.",
            level: "info",
            category: .settings
        )
        publishSnapshots()
    }

    func clearBehaviorLog() {
        state.clearBehavior()
        publishSnapshots()
    }

    func clearPenLog() {
        state.clearPen()
        publishSnapshots()
    }

    nonisolated func logAsync(
        _ message: String,
        level: String = "log",
        category: BehaviorLogCategory = .app
    ) {
        logBehavior(message, level: level, category: category)
    }

    nonisolated func logBehavior(
        _ message: String,
        level: String = "log",
        category: BehaviorLogCategory = .app
    ) {
        let trimmed = message.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else {
            return
        }

        state.appendBehavior(
            LogEntry(level: level, category: category, message: trimmed),
            maxEntries: behaviorLogMaxEntries
        )
        schedulePublish()
    }

    nonisolated func logPen(
        rawData: String,
        semantic: String,
        gestureState: PenGestureState?,
        capabilities: [String] = []
    ) {
        let trimmedRaw = rawData.trimmingCharacters(in: .whitespacesAndNewlines)
        let trimmedSemantic = semantic.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmedRaw.isEmpty else {
            return
        }

        let appended = state.appendPen(
            PenLogEntry(rawData: trimmedRaw, semantic: trimmedSemantic, gestureState: gestureState),
            capabilities: capabilities,
            maxEntries: penLogMaxEntries
        )
        guard appended else {
            return
        }
        schedulePublish()
    }

    nonisolated func beginPenSession(_ label: String) {
        state.beginPenSession(label: label)
        schedulePublish()
    }

    nonisolated func setPenSessionLabel(_ label: String?) {
        state.setPenSessionLabel(label)
        schedulePublish()
    }

    private func publishSnapshots() {
        let snapshot = state.snapshot()
        behaviorEntries = snapshot.behaviorEntries
        penEntries = snapshot.penEntries
        penCapabilityLabels = snapshot.penCapabilityLabels
        penLoggingEnabled = snapshot.penLoggingEnabled
        penSessionLabel = snapshot.penSessionLabel
    }

    nonisolated private func schedulePublish() {
        guard state.markPublishScheduled() else {
            return
        }

        Task { [weak self] in
            try? await Task.sleep(nanoseconds: 75_000_000)
            await self?.publishSnapshots()
        }
    }
}
