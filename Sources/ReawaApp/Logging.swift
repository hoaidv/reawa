import Foundation

struct LogEntry: Identifiable, Equatable, Sendable {
    let id = UUID()
    let timestamp = Date()
    let level: String
    let message: String

    var formatted: String {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss.SSS"
        return "\(formatter.string(from: timestamp))  \(message)"
    }
}

@MainActor
final class AppLogger: ObservableObject {
    static let shared = AppLogger()

    @Published private(set) var entries: [LogEntry] = []

    private let maxEntries = 2_000

    func log(_ message: String, level: String = "log") {
        let trimmed = message.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else {
            return
        }
        entries.append(LogEntry(level: level, message: trimmed))
        if entries.count > maxEntries {
            entries.removeFirst(entries.count - maxEntries)
        }
    }

    nonisolated func logAsync(_ message: String, level: String = "log") {
        Task { @MainActor in
            self.log(message, level: level)
        }
    }
}
