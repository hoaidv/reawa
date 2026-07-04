import Foundation
import Security

enum AppPaths {
    static let libraryURL = FileManager.default.homeDirectoryForCurrentUser
        .appendingPathComponent("Library", isDirectory: true)
    static let applicationSupportURL = libraryURL
        .appendingPathComponent("Application Support", isDirectory: true)
        .appendingPathComponent("Reawa", isDirectory: true)
    static let connectionsFileURL = applicationSupportURL.appendingPathComponent("connections.json")
    static let keysDirectoryURL = applicationSupportURL.appendingPathComponent("keys", isDirectory: true)

    static let legacyApplicationSupportURL = libraryURL
        .appendingPathComponent("Application Support", isDirectory: true)
        .appendingPathComponent("remarkable-rm2", isDirectory: true)
    static let legacyConnectionsFileURL = legacyApplicationSupportURL.appendingPathComponent("connections.json")
    static let legacyKeysDirectoryURL = legacyApplicationSupportURL.appendingPathComponent("keys", isDirectory: true)

    static func privateKeyURL(for connectionID: String) -> URL {
        keysDirectoryURL.appendingPathComponent(connectionID, isDirectory: true).appendingPathComponent("id_rsa")
    }

    static func publicKeyURL(for connectionID: String) -> URL {
        keysDirectoryURL.appendingPathComponent(connectionID, isDirectory: true).appendingPathComponent("id_rsa.pub")
    }
}

struct ConnectionStore {
    private let decoder = JSONDecoder()
    private let encoder = JSONEncoder()

    init() {
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        try? Self.prepareDirectories()
        migrateLegacyDataIfNeeded()
    }

    static func prepareDirectories() throws {
        try FileManager.default.createDirectory(at: AppPaths.applicationSupportURL, withIntermediateDirectories: true)
        try FileManager.default.createDirectory(at: AppPaths.keysDirectoryURL, withIntermediateDirectories: true)
    }

    func listConnections() -> [Connection] {
        guard let data = try? Data(contentsOf: AppPaths.connectionsFileURL) else {
            return []
        }
        return (try? decoder.decode(ConnectionFile.self, from: data).connections) ?? []
    }

    func get(_ connectionID: String) -> Connection? {
        listConnections().first(where: { $0.id == connectionID })
    }

    func add(_ connection: Connection) throws {
        var connections = listConnections()
        connections.append(connection)
        try saveConnections(connections)
    }

    func update(_ connection: Connection) throws {
        var connections = listConnections()
        guard let index = connections.firstIndex(where: { $0.id == connection.id }) else {
            throw NSError(domain: "Reawa.ConnectionStore", code: 404, userInfo: [NSLocalizedDescriptionKey: "Connection not found."])
        }
        connections[index] = connection
        try saveConnections(connections)
    }

    func remove(_ connectionID: String) throws {
        let filtered = listConnections().filter { $0.id != connectionID }
        try saveConnections(filtered)
        let keyDirectory = AppPaths.keysDirectoryURL.appendingPathComponent(connectionID, isDirectory: true)
        try? FileManager.default.removeItem(at: keyDirectory)
    }

    func saveConnections(_ connections: [Connection]) throws {
        let payload = ConnectionFile(connections: connections)
        let data = try encoder.encode(payload)
        var normalized = data
        normalized.append(0x0A)
        try normalized.write(to: AppPaths.connectionsFileURL, options: .atomic)
    }

    private func migrateLegacyDataIfNeeded() {
        guard !FileManager.default.fileExists(atPath: AppPaths.connectionsFileURL.path),
              FileManager.default.fileExists(atPath: AppPaths.legacyConnectionsFileURL.path)
        else {
            return
        }

        do {
            try FileManager.default.copyItem(at: AppPaths.legacyConnectionsFileURL, to: AppPaths.connectionsFileURL)
        } catch {
            return
        }

        guard FileManager.default.fileExists(atPath: AppPaths.legacyKeysDirectoryURL.path) else {
            return
        }

        let connectionIDs = (try? listConnectionIDs(in: AppPaths.legacyKeysDirectoryURL)) ?? []
        for id in connectionIDs {
            let legacyDir = AppPaths.legacyKeysDirectoryURL.appendingPathComponent(id, isDirectory: true)
            let newDir = AppPaths.keysDirectoryURL.appendingPathComponent(id, isDirectory: true)
            if FileManager.default.fileExists(atPath: newDir.path) {
                continue
            }
            try? FileManager.default.copyItem(at: legacyDir, to: newDir)
        }
    }

    private func listConnectionIDs(in directory: URL) throws -> [String] {
        try FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: [.isDirectoryKey])
            .filter { (try? $0.resourceValues(forKeys: [.isDirectoryKey]).isDirectory) == true }
            .map(\.lastPathComponent)
    }
}

struct KeychainStore {
    private let serviceName = "Reawa"
    private let legacyServiceName = "remarkable-rm2"

    func savePassword(_ password: String, for connectionID: String) throws {
        let data = Data(password.utf8)
        var query = baseQuery(service: serviceName, account: connectionID)
        let attributesToUpdate: [String: Any] = [kSecValueData as String: data]

        let status = SecItemCopyMatching(query as CFDictionary, nil)
        if status == errSecSuccess {
            let updateStatus = SecItemUpdate(query as CFDictionary, attributesToUpdate as CFDictionary)
            guard updateStatus == errSecSuccess else {
                throw keychainError(status: updateStatus)
            }
            return
        }

        query[kSecValueData as String] = data
        let addStatus = SecItemAdd(query as CFDictionary, nil)
        guard addStatus == errSecSuccess else {
            throw keychainError(status: addStatus)
        }
    }

    func getPassword(for connectionID: String) -> String? {
        if let current = readPassword(service: serviceName, account: connectionID) {
            return current
        }

        guard let legacy = readPassword(service: legacyServiceName, account: connectionID) else {
            return nil
        }

        try? savePassword(legacy, for: connectionID)
        try? deletePassword(for: connectionID, service: legacyServiceName)
        return legacy
    }

    func deletePassword(for connectionID: String) throws {
        try deletePassword(for: connectionID, service: serviceName)
        try deletePassword(for: connectionID, service: legacyServiceName)
    }

    private func deletePassword(for connectionID: String, service: String) throws {
        let status = SecItemDelete(baseQuery(service: service, account: connectionID) as CFDictionary)
        guard status == errSecSuccess || status == errSecItemNotFound else {
            throw keychainError(status: status)
        }
    }

    private func readPassword(service: String, account: String) -> String? {
        var query = baseQuery(service: service, account: account)
        query[kSecReturnData as String] = true
        query[kSecMatchLimit as String] = kSecMatchLimitOne

        var item: CFTypeRef?
        let status = SecItemCopyMatching(query as CFDictionary, &item)
        guard status == errSecSuccess,
              let data = item as? Data,
              let value = String(data: data, encoding: .utf8)
        else {
            return nil
        }
        return value
    }

    private func baseQuery(service: String, account: String) -> [String: Any] {
        [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
        ]
    }

    private func keychainError(status: OSStatus) -> Error {
        let message = SecCopyErrorMessageString(status, nil) as String? ?? "Unknown Keychain error"
        return NSError(domain: NSOSStatusErrorDomain, code: Int(status), userInfo: [NSLocalizedDescriptionKey: message])
    }
}
