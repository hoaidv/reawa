// Traceability (ADLC iter-000)
// @implements SRS-RW-04
// @implements SRS-RW-07
// @implements SRS-RW-20
// @implements SRS-RW-21
// @implements SRS-RW-22
// @implements SRS-RW-23
// @implements SRS-RW-24
// @implements SRS-RW-26
// @implements SRS-RW-27
// @implements SRS-RW-28

import Foundation
import Network
import UserNotifications

struct NetworkInterface: Equatable, Sendable {
    let name: String
    let address: String
    let netmask: String
    let network: String
    let broadcast: String
    let prefixLength: Int
}

enum NetworkDiscovery {
    private static let skippedInterfaces: Set<String> = ["lo0", "bridge0", "gif0", "stf0"]
    private static let sshPort: UInt16 = 22
    private static let maxScanHosts = 64
    private static let localSubnetPrefix = 24

    private final class ProbeState: @unchecked Sendable {
        var success = false
    }

    private final class FoundHosts: @unchecked Sendable {
        private let lock = NSLock()
        private(set) var hosts: Set<String> = []

        func insert(_ host: String) {
            lock.lock()
            hosts.insert(host)
            lock.unlock()
        }
    }

    static func isHostReachable(_ host: String, port: UInt16 = sshPort, timeout: TimeInterval = 1.0) -> Bool {
        let queue = DispatchQueue(label: "reawa.network.probe.\(host)")
        let connection = NWConnection(host: NWEndpoint.Host(host), port: NWEndpoint.Port(rawValue: port)!, using: .tcp)
        let semaphore = DispatchSemaphore(value: 0)
        let probeState = ProbeState()

        connection.stateUpdateHandler = { connectionState in
            switch connectionState {
            case .ready:
                probeState.success = true
                connection.cancel()
                semaphore.signal()
            case .failed, .cancelled:
                semaphore.signal()
            default:
                break
            }
        }

        connection.start(queue: queue)
        let _ = semaphore.wait(timeout: .now() + timeout)
        connection.cancel()
        return probeState.success
    }

    static func listNetworkInterfaces() -> [NetworkInterface] {
        guard let output = try? ProcessRunner.run(launchPath: "/sbin/ifconfig", arguments: []) else {
            return []
        }

        var interfaces: [NetworkInterface] = []
        var current: String?

        for line in output.split(separator: "\n", omittingEmptySubsequences: false) {
            let text = String(line)
            if let first = text.first, !first.isWhitespace {
                current = text.split(separator: ":").first.map(String.init)
                continue
            }

            guard let current, !skippedInterfaces.contains(current), !current.hasPrefix("utun") else {
                continue
            }

            let trimmed = text.trimmingCharacters(in: .whitespaces)
            guard trimmed.hasPrefix("inet ") else {
                continue
            }

            let parts = trimmed.split(separator: " ").map(String.init)
            guard parts.count >= 4,
                  let maskIndex = parts.firstIndex(of: "netmask"),
                  parts.indices.contains(maskIndex + 1)
            else {
                continue
            }

            let address = parts[1]
            if address.contains(":") {
                continue
            }

            let netmask = parts[maskIndex + 1]
            let prefix = maskToPrefix(netmask)
            let addressInt = ipv4ToInt(address)
            let maskInt = prefix == 0 ? 0 : UInt32.max << (32 - UInt32(prefix))
            let networkInt = addressInt & maskInt
            let broadcastInt = networkInt | (~maskInt)

            interfaces.append(
                NetworkInterface(
                    name: current,
                    address: address,
                    netmask: netmask,
                    network: intToIPv4(networkInt),
                    broadcast: intToIPv4(broadcastInt),
                    prefixLength: prefix
                )
            )
        }

        return interfaces
    }

    static func discoverUSBSSHHosts() -> Set<String> {
        let interfaces = listNetworkInterfaces()
        let usbLike = interfaces.filter { $0.name.hasPrefix("en") && !["en0", "en1", "en2", "en3"].contains($0.name) }
        if !usbLike.isEmpty {
            return discoverSSHHosts(interfaces: usbLike)
        }
        return discoverSSHHosts(interfaces: interfaces)
    }

    static func discoverSSHHosts(interfaces: [NetworkInterface]? = nil) -> Set<String> {
        let candidates = candidateIPs(interfaces: interfaces ?? listNetworkInterfaces())
        guard !candidates.isEmpty else {
            return []
        }

        let queue = DispatchQueue(label: "reawa.network.discovery", attributes: .concurrent)
        let group = DispatchGroup()
        let foundHosts = FoundHosts()

        for host in candidates {
            group.enter()
            queue.async {
                defer { group.leave() }
                if isHostReachable(host, port: sshPort, timeout: 0.35) {
                    foundHosts.insert(host)
                }
            }
        }

        group.wait()
        return foundHosts.hosts
    }

    private static func candidateIPs(interfaces: [NetworkInterface]) -> [String] {
        var seen: Set<String> = []
        var ordered: [String] = []

        for interface in interfaces {
            for host in candidateIPs(interface: interface) where seen.insert(host).inserted {
                ordered.append(host)
            }
        }

        return ordered
    }

    static func candidateIPs(interface: NetworkInterface) -> [String] {
        let network = ipv4ToInt(interface.network)
        let broadcast = ipv4ToInt(interface.broadcast)
        let address = ipv4ToInt(interface.address)
        let hostCount = Int(broadcast &- network &- 1)
        guard hostCount > 0 else {
            return []
        }

        var candidates: [String] = []
        var seen: Set<String> = []

        func append(_ ip: String) {
            guard ip != interface.address, ip != interface.broadcast else {
                return
            }
            if seen.insert(ip).inserted {
                candidates.append(ip)
            }
        }

        if interface.prefixLength < localSubnetPrefix {
            let localMask = UInt32.max << (32 - UInt32(localSubnetPrefix))
            let localNetwork = address & localMask
            let localBroadcast = localNetwork | (~localMask)

            let localStart = Int(localNetwork) + 1
            let localEnd = Int(localBroadcast) - 1
            let localAddress = Int(address)
            let maxDistance = max(localAddress - localStart, localEnd - localAddress)
            if maxDistance > 0 {
                for distance in 1...maxDistance {
                    let lower = localAddress - distance
                    let upper = localAddress + distance
                    if lower >= localStart {
                        append(intToIPv4(UInt32(lower)))
                    }
                    if upper <= localEnd {
                        append(intToIPv4(UInt32(upper)))
                    }
                }
            }
        }

        let gateway = intToIPv4(network + 1)
        append(gateway)

        let limit = min(hostCount, maxScanHosts)
        for offset in 1...limit {
            append(intToIPv4(network + UInt32(offset)))
        }

        return candidates
    }

    private static func maskToPrefix(_ mask: String) -> Int {
        if mask.hasPrefix("0x"), let value = UInt32(mask.dropFirst(2), radix: 16) {
            return value.nonzeroBitCount
        }

        let parts = mask.split(separator: ".").compactMap { UInt32($0) }
        guard parts.count == 4 else {
            return 0
        }
        let value = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3]
        return value.nonzeroBitCount
    }

    private static func ipv4ToInt(_ ip: String) -> UInt32 {
        let parts = ip.split(separator: ".").compactMap { UInt32($0) }
        guard parts.count == 4 else {
            return 0
        }
        return (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3]
    }

    private static func intToIPv4(_ value: UInt32) -> String {
        [
            String((value >> 24) & 0xFF),
            String((value >> 16) & 0xFF),
            String((value >> 8) & 0xFF),
            String(value & 0xFF),
        ].joined(separator: ".")
    }
}

@MainActor
final class NotificationService {
    private let logger: AppLogger
    private let canUseUserNotifications: Bool

    init(logger: AppLogger) {
        self.logger = logger
        canUseUserNotifications = Self.supportsUserNotifications(bundleURL: Bundle.main.bundleURL)
        guard canUseUserNotifications else {
            logger.log("Notifications disabled for non-bundled runs.", level: "info", category: .notification)
            return
        }
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound]) { _, _ in }
    }

    func send(title: String, body: String) {
        guard canUseUserNotifications else {
            logger.log("notification suppressed outside app bundle: \(title) - \(body)", level: "info", category: .notification)
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        let request = UNNotificationRequest(identifier: UUID().uuidString, content: content, trigger: nil)
        UNUserNotificationCenter.current().add(request)
        logger.log("notification: \(title) - \(body)", level: "info", category: .notification)
    }

    nonisolated static func supportsUserNotifications(bundleURL: URL) -> Bool {
        bundleURL.pathExtension.caseInsensitiveCompare("app") == .orderedSame
    }
}

@MainActor
final class USBWatcher {
    private struct PollResult: Sendable {
        let discovered: Set<String>
        let reachable: Set<String>
    }

    private var timer: Timer?
    private var pollTask: Task<Void, Never>?
    private var wasReachable: Set<String> = []
    private var wasDiscoveredIPs: Set<String> = []
    private weak var manager: ConnectionManager?
    private weak var notifications: NotificationService?
    private let logger: AppLogger
    private let interval: TimeInterval
    private var onDetected: ((Connection) -> Void)?

    init(manager: ConnectionManager, notifications: NotificationService, logger: AppLogger, interval: TimeInterval = 3.0) {
        self.manager = manager
        self.notifications = notifications
        self.logger = logger
        self.interval = interval
    }

    func setOnDetected(_ callback: @escaping (Connection) -> Void) {
        onDetected = callback
    }

    func start() {
        guard timer == nil else {
            return
        }
        let timer = Timer.scheduledTimer(withTimeInterval: interval, repeats: true) { [weak self] _ in
            Task { @MainActor in
                self?.schedulePoll()
            }
        }
        timer.tolerance = min(1.0, interval / 3.0)
        self.timer = timer
        schedulePoll()
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        pollTask?.cancel()
        pollTask = nil
    }

    private func schedulePoll() {
        guard pollTask == nil, let manager else {
            return
        }

        let connections = manager.connections
        pollTask = Task { [weak self] in
            let result = await Task.detached(priority: .utility) {
                USBWatcher.computePollResult(connections: connections)
            }.value

            guard !Task.isCancelled else {
                return
            }

            await MainActor.run {
                self?.applyPoll(result: result)
                self?.pollTask = nil
            }
        }
    }

    private func applyPoll(result: PollResult) {
        guard let manager else {
            return
        }

        let previousReachable = wasReachable
        let previousDiscoveredIPs = wasDiscoveredIPs
        manager.setDiscoveredIPs(result.discovered)
        let connections = manager.connections
        let currentReachable = result.reachable

        manager.updateReachability(currentReachable)

        if !connections.isEmpty {
            let newlyOnline = currentReachable.subtracting(previousReachable)
            for connectionID in newlyOnline {
                guard let connection = connections.first(where: { $0.id == connectionID }) else {
                    continue
                }
                if connection.autoConnect {
                    do {
                        logger.log("Detected \(connection.name) on USB; attempting auto-connect.", level: "info", category: .discovery)
                        try manager.connect(connectionID)
                        notifications?.send(title: "Reawa Connected", body: "Auto-connected to \(connection.name)")
                    } catch {
                        logger.log("Auto-connect failed for \(connection.name): \(error.localizedDescription)", level: "error", category: .discovery)
                        notifications?.send(title: "Reawa Connection Failed", body: "\(connection.name): \(error.localizedDescription)")
                    }
                } else {
                    logger.log("Detected saved device \(connection.name) at \(connection.ip).", level: "info", category: .discovery)
                    notifications?.send(title: "Reawa Detected", body: "\(connection.name) is available — open the app to connect")
                    onDetected?(connection)
                }
            }

            let wentOffline = previousReachable.subtracting(currentReachable)
            if let activeID = manager.activeConnectionID, wentOffline.contains(activeID) {
                let name = manager.connection(id: activeID)?.name ?? activeID
                logger.log("Active device \(name) went offline.", level: "info", category: .discovery)
                manager.disconnect()
                notifications?.send(title: "Reawa Disconnected", body: "\(name) is no longer reachable")
            }
        } else {
            let newIPs = result.discovered.subtracting(previousDiscoveredIPs)
            if !newIPs.isEmpty {
                logger.log("Detected SSH-capable device(s): \(newIPs.sorted().joined(separator: ", ")).", level: "info", category: .discovery)
                notifications?.send(title: "Reawa Detected", body: "SSH device at \(newIPs.sorted().joined(separator: ", ")) — add a connection in Open")
            }
        }

        wasReachable = currentReachable
        wasDiscoveredIPs = result.discovered
    }

    nonisolated private static func computeDiscoveredIPs() -> Set<String> {
        let usb = NetworkDiscovery.discoverUSBSSHHosts()
        if !usb.isEmpty {
            return usb
        }
        return NetworkDiscovery.discoverSSHHosts()
    }

    nonisolated private static func computePollResult(connections: [Connection]) -> PollResult {
        let discovered = computeDiscoveredIPs()
        let reachable = Set(connections.compactMap { connection in
            if discovered.contains(connection.ip) || NetworkDiscovery.isHostReachable(connection.ip) {
                return connection.id
            }
            return nil
        })
        return PollResult(discovered: discovered, reachable: reachable)
    }
}
