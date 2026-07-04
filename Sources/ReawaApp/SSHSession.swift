import Foundation

private extension Data {
    func uint32LE(at offset: Int) -> UInt32 {
        withUnsafeBytes { rawBuffer in
            UInt32(littleEndian: rawBuffer.load(fromByteOffset: offset, as: UInt32.self))
        }
    }

    func uint16LE(at offset: Int) -> UInt16 {
        withUnsafeBytes { rawBuffer in
            UInt16(littleEndian: rawBuffer.load(fromByteOffset: offset, as: UInt16.self))
        }
    }

    func int32LE(at offset: Int) -> Int32 {
        withUnsafeBytes { rawBuffer in
            Int32(littleEndian: rawBuffer.load(fromByteOffset: offset, as: Int32.self))
        }
    }
}

enum DriverSessionEvent: Sendable {
    case connected
    case stopped
    case failed(String)
    case nativeStylusStatus(NativeStylusStatus?)
    case modeFallback(requested: OutputMode, active: OutputMode, reason: String)
}

struct StreamProcess {
    let process: Process
    let stdout: FileHandle
    let stderr: Pipe
}

enum SSHKeyInstaller {
    static func ensureKeyPair(at privateKeyURL: URL) throws {
        guard !FileManager.default.fileExists(atPath: privateKeyURL.path) else {
            return
        }

        try FileManager.default.createDirectory(at: privateKeyURL.deletingLastPathComponent(), withIntermediateDirectories: true)
        _ = try ProcessRunner.run(
            launchPath: "/usr/bin/ssh-keygen",
            arguments: [
                "-t", "rsa",
                "-b", String(RM2.sshKeyBits),
                "-N", "",
                "-C", RM2.sshKeyComment,
                "-f", privateKeyURL.path,
            ]
        )
    }

    static func connectPenStream(
        ip: String,
        keyURL: URL,
        password: String?,
        logger: AppLogger
    ) throws -> StreamProcess {
        try ensureKeyPair(at: keyURL)

        logger.logAsync("[ssh] attempting key auth for \(ip)", level: "info", category: .ssh)
        do {
            try preflightKeyConnection(ip: ip, keyURL: keyURL)
            logger.logAsync("[ssh] key auth succeeded for \(ip)", level: "info", category: .ssh)
        } catch {
            guard let password, !password.isEmpty else {
                throw error
            }
            logger.logAsync("[ssh] key auth failed for \(ip); installing the public key with password auth.", level: "info", category: .ssh)
            try setupKey(ip: ip, password: password, keyURL: keyURL, logger: logger)
            try preflightKeyConnection(ip: ip, keyURL: keyURL)
            logger.logAsync("[ssh] password-assisted key setup succeeded for \(ip)", level: "info", category: .ssh)
        }

        let stdout = Pipe()
        let stderr = Pipe()
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/ssh")
        process.arguments = sshArguments(
            ip: ip,
            keyURL: keyURL,
            remoteCommand: "dd bs=\(RM2.eventSize) if=\(RM2.penFile) 2>/dev/null"
        )
        process.standardOutput = stdout
        process.standardError = stderr
        try process.run()

        return StreamProcess(process: process, stdout: stdout.fileHandleForReading, stderr: stderr)
    }

    static func setupKey(ip: String, password: String, keyURL: URL, logger: AppLogger? = nil) throws {
        let pubkeyURL = keyURL.appendingPathExtension("pub")
        let pubkeyLine = try String(contentsOf: pubkeyURL, encoding: .utf8).trimmingCharacters(in: .whitespacesAndNewlines)
        let escapedKey = shellEscapeSingleQuotes(pubkeyLine)
        let remoteScript = """
        SSH_DIR="$HOME/.ssh"
        AUTH_KEYS="$SSH_DIR/authorized_keys"
        mkdir -p "$SSH_DIR"
        chmod 700 "$SSH_DIR"
        touch "$AUTH_KEYS"
        chmod 600 "$AUTH_KEYS"
        grep -qxF '\(escapedKey)' "$AUTH_KEYS" 2>/dev/null || echo '\(escapedKey)' >> "$AUTH_KEYS"
        """

        let askpassURL = try writeAskpassScript(password: password)
        defer { try? FileManager.default.removeItem(at: askpassURL) }

        logger?.logAsync("[ssh] installing public key on \(ip) with password auth", level: "info", category: .ssh)
        _ = try ProcessRunner.run(
            launchPath: "/usr/bin/ssh",
            arguments: [
                "-o", "PreferredAuthentications=password",
                "-o", "PubkeyAuthentication=no",
                "-o", "StrictHostKeyChecking=no",
                "-o", "UserKnownHostsFile=/dev/null",
                "-o", "NumberOfPasswordPrompts=1",
                "\(RM2.user)@\(ip)",
                remoteScript,
            ],
            environment: [
                "SSH_ASKPASS": askpassURL.path,
                "SSH_ASKPASS_REQUIRE": "force",
                "DISPLAY": "1",
            ]
        )
        logger?.logAsync("[ssh] installed public key on \(ip)", level: "info", category: .ssh)
    }

    private static func preflightKeyConnection(ip: String, keyURL: URL) throws {
        _ = try ProcessRunner.run(
            launchPath: "/usr/bin/ssh",
            arguments: sshArguments(ip: ip, keyURL: keyURL, remoteCommand: "true")
        )
    }

    private static func sshArguments(ip: String, keyURL: URL, remoteCommand: String) -> [String] {
        [
            "-o", "BatchMode=yes",
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=/dev/null",
            "-i", keyURL.path,
            "\(RM2.user)@\(ip)",
            remoteCommand,
        ]
    }

    private static func writeAskpassScript(password: String) throws -> URL {
        let scriptURL = FileManager.default.temporaryDirectory.appendingPathComponent("reawa-askpass-\(UUID().uuidString).sh")
        let script = """
        #!/bin/sh
        printf '%s\n' '\(shellEscapeSingleQuotes(password))'
        """
        try script.write(to: scriptURL, atomically: true, encoding: .utf8)
        try FileManager.default.setAttributes([.posixPermissions: 0o700], ofItemAtPath: scriptURL.path)
        return scriptURL
    }
}

private enum PenLogFormatter {
    static func semanticDescription(rawEvent: PenRawEvent, snapshot: PenStateSnapshot) -> String {
        if rawEvent.type == LinuxInputEventType.key.rawValue, rawEvent.code == LinuxInputCode.btnStylus {
            return "STYLUS BUTTON \(snapshot.stylusButton ? "DOWN" : "UP")" + stateSuffix(snapshot: snapshot)
        }

        if !snapshot.inProximity {
            return "PEN OUT"
        }

        let prefix = snapshot.touching ? "PEN TOUCH" : "PEN HOVER"
        var details = ["\(prefix) \(positionText(snapshot: snapshot))"]

        if let pressure = snapshot.pressure {
            details.append("pressure=\(pressure)")
        }
        if let distance = snapshot.distance {
            details.append("distance=\(distance)")
        }
        if let tiltX = snapshot.tiltX, let tiltY = snapshot.tiltY {
            details.append("tilt=(\(tiltX), \(tiltY))")
        } else if let tiltX = snapshot.tiltX {
            details.append("tiltX=\(tiltX)")
        } else if let tiltY = snapshot.tiltY {
            details.append("tiltY=\(tiltY)")
        }
        if snapshot.stylusButton {
            details.append("stylus=down")
        }

        return details.joined(separator: " ")
    }

    private static func positionText(snapshot: PenStateSnapshot) -> String {
        let x = snapshot.x.map(String.init) ?? "?"
        let y = snapshot.y.map(String.init) ?? "?"
        return "(x, y) = (\(x), \(y))"
    }

    private static func stateSuffix(snapshot: PenStateSnapshot) -> String {
        guard snapshot.inProximity else {
            return ""
        }
        return " · " + (snapshot.touching ? "PEN TOUCH" : "PEN HOVER") + " " + positionText(snapshot: snapshot)
    }
}

struct PenFrameParser {
    private var buffer = Data()
    private var state = PenStateSnapshot()
    private var pendingRawEvents: [PenRawEvent] = []
    var onRawEvent: ((PenRawEvent, PenStateSnapshot, PenGestureState?) -> Void)?

    mutating func append(_ data: Data) -> [PenFrame] {
        buffer.append(data)
        var frames: [PenFrame] = []

        while buffer.count >= RM2.eventSize {
            let event = buffer.prefix(RM2.eventSize)
            buffer.removeFirst(RM2.eventSize)
            if let frame = parse(event) {
                frames.append(frame)
            }
        }

        return frames
    }

    private mutating func parse(_ event: Data.SubSequence) -> PenFrame? {
        let data = Data(event)
        let rawEvent = PenRawEvent(
            tvSec: data.uint32LE(at: 0),
            tvUsec: data.uint32LE(at: 4),
            type: data.uint16LE(at: 8),
            code: data.uint16LE(at: 10),
            value: Int(data.int32LE(at: 12))
        )

        pendingRawEvents.append(rawEvent)
        let gestureState = apply(rawEvent)

        if rawEvent.type != LinuxInputEventType.syn.rawValue {
            onRawEvent?(rawEvent, state, gestureState)
        }

        if rawEvent.type == LinuxInputEventType.syn.rawValue, rawEvent.code == LinuxInputCode.synReport {
            defer { pendingRawEvents.removeAll(keepingCapacity: true) }
            if let penX = state.x, let penY = state.y {
                return PenFrame(
                    tvSec: rawEvent.tvSec,
                    tvUsec: rawEvent.tvUsec,
                    x: penX,
                    y: penY,
                    pressure: state.pressure,
                    touching: state.touching,
                    inProximity: state.inProximity,
                    stylusButton: state.stylusButton,
                    distance: state.distance,
                    tiltX: state.tiltX,
                    tiltY: state.tiltY,
                    rawEvents: pendingRawEvents
                )
            }
        }

        return nil
    }

    private mutating func apply(_ rawEvent: PenRawEvent) -> PenGestureState? {
        switch rawEvent.type {
        case LinuxInputEventType.abs.rawValue:
            switch rawEvent.code {
            case LinuxInputCode.absX:
                state.x = rawEvent.value
                return state.inProximity ? .move : nil
            case LinuxInputCode.absY:
                state.y = rawEvent.value
                return state.inProximity ? .move : nil
            case LinuxInputCode.absPressure:
                state.pressure = rawEvent.value
                return state.inProximity ? .move : nil
            case LinuxInputCode.absDistance:
                state.distance = rawEvent.value
                return state.inProximity ? .move : nil
            case LinuxInputCode.absTiltX:
                state.tiltX = rawEvent.value
                return state.inProximity ? .move : nil
            case LinuxInputCode.absTiltY:
                state.tiltY = rawEvent.value
                return state.inProximity ? .move : nil
            default:
                return nil
            }
        case LinuxInputEventType.key.rawValue:
            switch rawEvent.code {
            case LinuxInputCode.btnTouch:
                state.touching = rawEvent.value == 1
                return state.touching ? .start : .end
            case LinuxInputCode.btnToolPen:
                state.inProximity = rawEvent.value == 1
                if !state.inProximity {
                    state.touching = false
                }
                return state.inProximity ? .start : .out
            case LinuxInputCode.btnStylus:
                state.stylusButton = rawEvent.value == 1
                return nil
            default:
                return nil
            }
        default:
            return nil
        }
    }
}

final class DriverSession: @unchecked Sendable {
    private let connection: Connection
    private let keyURL: URL
    private let password: String?
    private let logger: AppLogger
    private let onEvent: @MainActor @Sendable (DriverSessionEvent) -> Void

    private let lock = NSLock()
    private var currentConfig: DeviceConfig
    private var stopRequested = false
    private var paused = false
    private var thread: Thread?
    private var process: Process?

    init(
        connection: Connection,
        keyURL: URL,
        password: String?,
        logger: AppLogger,
        onEvent: @escaping @MainActor @Sendable (DriverSessionEvent) -> Void
    ) {
        self.connection = connection
        self.keyURL = keyURL
        self.password = password
        self.logger = logger
        self.onEvent = onEvent
        currentConfig = connection.deviceConfig
    }

    func start() {
        guard thread == nil else {
            return
        }

        stopRequested = false
        let worker = Thread { [weak self] in
            self?.run()
        }
        worker.name = "Reawa Driver Session"
        thread = worker
        worker.start()
    }

    func stop() {
        lock.lock()
        stopRequested = true
        let process = self.process
        self.process = nil
        thread = nil
        lock.unlock()

        process?.terminate()
    }

    func pause() {
        lock.lock()
        paused = true
        lock.unlock()
    }

    func resume() {
        lock.lock()
        paused = false
        lock.unlock()
    }

    func updateConfig(_ config: DeviceConfig) {
        lock.lock()
        currentConfig = config
        lock.unlock()
    }

    private func run() {
        var streamProcess: StreamProcess?
        var activeBackend: PenOutputBackend?

        do {
            logger.logAsync("[session] connecting to \(connection.name) (\(connection.ip))…", level: "info", category: .session)
            streamProcess = try SSHKeyInstaller.connectPenStream(ip: connection.ip, keyURL: keyURL, password: password, logger: logger)
            setProcess(streamProcess?.process)
            logger.beginPenSession(connection.name)
            emit(.connected)

            var config = snapshotConfig()
            var currentMode = config.outputMode
            var lastMouseMode = currentMode.isMouseEmulation ? currentMode : .relative
            let initialSelection = makeBackend(mode: currentMode, config: config, fallbackMode: lastMouseMode)
            activeBackend = initialSelection.backend
            var parser = PenFrameParser()
            parser.onRawEvent = { [logger] rawEvent, snapshot, gestureState in
                logger.logPen(
                    rawData: rawEvent.rawDataText,
                    semantic: PenLogFormatter.semanticDescription(rawEvent: rawEvent, snapshot: snapshot),
                    gestureState: gestureState,
                    capabilities: rawEvent.capabilityLabels
                )
            }

            while !shouldStop() {
                let data = try streamProcess?.stdout.read(upToCount: RM2.eventSize * 8) ?? Data()
                if data.isEmpty {
                    break
                }

                for frame in parser.append(data) {
                    if shouldStop() {
                        break
                    }

                    if isPaused() {
                        activeBackend?.cleanup()
                        continue
                    }

                    config = snapshotConfig()
                    if config.outputMode.isMouseEmulation {
                        lastMouseMode = config.outputMode
                    }

                    if config.outputMode != currentMode {
                        activeBackend?.cleanup()
                        logger.logBehavior(
                            "[session] output mode changed from \(currentMode.rawValue) to \(config.outputMode.rawValue)",
                            level: "info",
                            category: .mode
                        )
                        let selection = makeBackend(mode: config.outputMode, config: config, fallbackMode: lastMouseMode)
                        currentMode = config.outputMode
                        activeBackend = selection.backend
                        if config.outputMode.isMouseEmulation {
                            emit(.nativeStylusStatus(nil))
                        }
                    }

                    activeBackend?.updateConfig(config)
                    activeBackend?.handle(frame: frame)
                }
            }

            activeBackend?.cleanup()
            emit(.nativeStylusStatus(nil))
            if let process = streamProcess?.process {
                process.waitUntilExit()
                if process.terminationStatus != 0, !shouldStop() {
                    let stderr = String(data: streamProcess?.stderr.fileHandleForReading.readDataToEndOfFile() ?? Data(), encoding: .utf8) ?? ""
                    throw ProcessError.nonZeroExit(code: process.terminationStatus, stderr: stderr)
                }
            }

            if !shouldStop() {
                emit(.stopped)
            }
        } catch {
            activeBackend?.cleanup()
            emit(.nativeStylusStatus(nil))
            if !shouldStop() {
                logger.logAsync("[session] error: \(error.localizedDescription)", level: "error", category: .session)
                emit(.failed(error.localizedDescription))
            }
        }

        setProcess(nil)
        lock.lock()
        thread = nil
        lock.unlock()
    }

    private func makeBackend(
        mode: OutputMode,
        config: DeviceConfig,
        fallbackMode: OutputMode
    ) -> (backend: PenOutputBackend, activeMode: OutputMode) {
        switch mode {
        case .absolute:
            return (
                AbsolutePenDriver(mouse: MouseController(config: config), region: config.absolute),
                .absolute
            )
        case .relative:
            return (
                RelativePenDriver(mouse: MouseController(config: config)),
                .relative
            )
        case .nativeStylus:
            do {
                let backend = try NativeStylusBackend(
                    config: config,
                    logger: logger,
                    onStatus: { [weak self] status in
                        self?.emit(.nativeStylusStatus(status))
                    }
                )
                return (backend, .nativeStylus)
            } catch {
                let resolvedFallback = fallbackMode.isMouseEmulation ? fallbackMode : .relative
                logger.logAsync(
                    "Native Stylus unavailable: \(error.localizedDescription). Falling back to \(resolvedFallback.title).",
                    level: "error",
                    category: .mode
                )
                emit(
                    .modeFallback(
                        requested: .nativeStylus,
                        active: resolvedFallback,
                        reason: error.localizedDescription
                    )
                )
                emit(
                    .nativeStylusStatus(
                        NativeStylusStatus(kind: .error, message: error.localizedDescription)
                    )
                )
                return makeBackend(mode: resolvedFallback, config: config, fallbackMode: .relative)
            }
        }
    }

    private func emit(_ event: DriverSessionEvent) {
        let onEvent = self.onEvent
        Task { @MainActor in
            onEvent(event)
        }
    }

    private func snapshotConfig() -> DeviceConfig {
        lock.lock()
        defer { lock.unlock() }
        return currentConfig
    }

    private func shouldStop() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        return stopRequested
    }

    private func isPaused() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        return paused
    }

    private func setProcess(_ process: Process?) {
        lock.lock()
        self.process = process
        lock.unlock()
    }
}
