import Foundation

@MainActor
final class ConnectionManager: ObservableObject {
    @Published private(set) var connections: [Connection]
    @Published private(set) var discoveredIPs: [String] = []
    @Published private(set) var activeConnectionID: String?

    private let store = ConnectionStore()
    private let keychain = KeychainStore()
    private let logger: AppLogger

    private var reachableIDs: Set<String> = []
    private var errors: [String: String] = [:]
    private var activeSession: DriverSession?
    private var activeSessionConnected = false

    init(logger: AppLogger) {
        self.logger = logger
        connections = store.listConnections()
    }

    func refreshConnections() {
        connections = store.listConnections()
    }

    func connection(id: String) -> Connection? {
        connections.first(where: { $0.id == id })
    }

    func status(for connectionID: String) -> ConnectionStatus {
        if activeConnectionID == connectionID, activeSessionConnected {
            return .connected
        }
        if errors[connectionID] != nil {
            return .error
        }
        if reachableIDs.contains(connectionID) {
            return .online
        }
        return .offline
    }

    func errorMessage(for connectionID: String) -> String? {
        errors[connectionID]
    }

    func setDiscoveredIPs(_ ips: Set<String>) {
        discoveredIPs = ips.sorted()
        objectWillChange.send()
    }

    func updateReachability(_ reachable: Set<String>) {
        let newlyOnline = reachable.subtracting(reachableIDs)
        let wentOffline = reachableIDs.subtracting(reachable)
        for connectionID in newlyOnline {
            if let connection = connection(id: connectionID) {
                logger.log("\(connection.name) is reachable at \(connection.ip).", level: "info", category: .connection)
            }
        }
        for connectionID in wentOffline {
            if let connection = connection(id: connectionID) {
                logger.log("\(connection.name) is no longer reachable.", level: "info", category: .connection)
            }
        }
        reachableIDs = reachable
        for connectionID in wentOffline {
            errors.removeValue(forKey: connectionID)
        }
        objectWillChange.send()
    }

    func addConnection(
        name: String,
        ip: String,
        password: String,
        autoConnect: Bool
    ) async throws -> Connection {
        let connection = Connection(name: name, ip: ip, autoConnect: autoConnect)
        try store.add(connection)
        do {
            try keychain.savePassword(password, for: connection.id)
            try SSHKeyInstaller.setupKey(ip: ip, password: password, keyURL: AppPaths.privateKeyURL(for: connection.id), logger: logger)
            refreshConnections()
            logger.log("Added connection \(connection.name) (\(connection.ip)).", level: "info", category: .connection)
            return connection
        } catch {
            try? store.remove(connection.id)
            try? keychain.deletePassword(for: connection.id)
            refreshConnections()
            throw error
        }
    }

    func updateConnection(_ connection: Connection) {
        do {
            try store.update(connection)
            refreshConnections()
            if activeConnectionID == connection.id {
                activeSession?.updateConfig(connection.deviceConfig)
            }
        } catch {
            logger.log("Failed to update \(connection.id): \(error.localizedDescription)", level: "error", category: .connection)
        }
    }

    func removeConnection(_ connectionID: String) {
        if activeConnectionID == connectionID {
            disconnect()
        }

        do {
            try store.remove(connectionID)
            try? keychain.deletePassword(for: connectionID)
            reachableIDs.remove(connectionID)
            errors.removeValue(forKey: connectionID)
            refreshConnections()
            logger.log("Removed connection \(connectionID).", level: "info", category: .connection)
        } catch {
            logger.log("Failed to remove \(connectionID): \(error.localizedDescription)", level: "error", category: .connection)
        }
    }

    func connect(_ connectionID: String) throws {
        if activeConnectionID == connectionID, activeSessionConnected {
            return
        }

        if let activeConnectionID, activeConnectionID != connectionID {
            if let activeConnection = connection(id: activeConnectionID) {
                logger.log("Switching active connection from \(activeConnection.name).", level: "info", category: .connection)
            }
            disconnect()
        }

        guard let connection = connection(id: connectionID) else {
            throw NSError(domain: "Reawa.ConnectionManager", code: 404, userInfo: [NSLocalizedDescriptionKey: "Unknown connection: \(connectionID)"])
        }

        errors.removeValue(forKey: connectionID)
        activeConnectionID = connectionID
        activeSessionConnected = false
        logger.log("Attempting connection to \(connection.name) (\(connection.ip)).", level: "info", category: .connection)

        let keyURL = AppPaths.privateKeyURL(for: connectionID)
        let password = FileManager.default.fileExists(atPath: keyURL.path) ? nil : keychain.getPassword(for: connectionID)
        let handler: @MainActor @Sendable (DriverSessionEvent) -> Void = { [weak self] event in
            self?.handleSessionEvent(event, for: connectionID)
        }
        let session = DriverSession(
            connection: connection,
            keyURL: keyURL,
            password: password,
            logger: logger,
            onEvent: handler
        )
        activeSession = session
        session.start()
        objectWillChange.send()
    }

    func disconnect() {
        if let activeConnectionID, let connection = connection(id: activeConnectionID) {
            logger.log("Disconnecting from \(connection.name).", level: "info", category: .connection)
        }
        activeSession?.stop()
        activeSession = nil
        activeConnectionID = nil
        activeSessionConnected = false
        objectWillChange.send()
    }

    func toggleConnection(_ connectionID: String) throws {
        if activeConnectionID == connectionID, activeSessionConnected {
            disconnect()
        } else {
            try connect(connectionID)
        }
    }

    func pauseInput() {
        activeSession?.pause()
    }

    func resumeInput() {
        activeSession?.resume()
    }

    private func handleSessionEvent(_ event: DriverSessionEvent, for connectionID: String) {
        switch event {
        case .connected:
            activeSessionConnected = true
            let name = connection(id: connectionID)?.name ?? connectionID
            logger.log("[session] connected to \(name)", level: "info", category: .session)
        case .stopped:
            activeSessionConnected = false
            if activeConnectionID == connectionID {
                activeConnectionID = nil
                activeSession = nil
            }
            let name = connection(id: connectionID)?.name ?? connectionID
            logger.log("[session] stopped for \(name)", level: "info", category: .session)
        case let .failed(message):
            activeSessionConnected = false
            errors[connectionID] = message
            if activeConnectionID == connectionID {
                activeConnectionID = nil
                activeSession = nil
            }
            let name = connection(id: connectionID)?.name ?? connectionID
            logger.log("[session] failed for \(name): \(message)", level: "error", category: .session)
        }
        objectWillChange.send()
    }
}
