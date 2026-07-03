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

        do {
            try preflightKeyConnection(ip: ip, keyURL: keyURL)
        } catch {
            guard let password, !password.isEmpty else {
                throw error
            }
            logger.logAsync("SSH key auth failed for \(ip); installing the public key with password auth.", level: "info")
            try setupKey(ip: ip, password: password, keyURL: keyURL)
            try preflightKeyConnection(ip: ip, keyURL: keyURL)
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

    static func setupKey(ip: String, password: String, keyURL: URL) throws {
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

struct PenFrameParser {
    private var buffer = Data()
    private var penX: Int?
    private var penY: Int?
    private var pressure: Int?
    private var touching = false
    private var inProximity = false

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
        let tvSec = data.uint32LE(at: 0)
        let tvUsec = data.uint32LE(at: 4)
        let type = data.uint16LE(at: 8)
        let code = data.uint16LE(at: 10)
        let value = Int(data.int32LE(at: 12))

        switch type {
        case 3:
            switch code {
            case 0: penX = value
            case 1: penY = value
            case 24: pressure = value
            default: break
            }
        case 1:
            switch code {
            case 330: touching = value == 1
            case 320: inProximity = value == 1
            default: break
            }
        case 0 where code == 0:
            if let penX, let penY {
                return PenFrame(
                    tvSec: tvSec,
                    tvUsec: tvUsec,
                    x: penX,
                    y: penY,
                    pressure: pressure,
                    touching: touching,
                    inProximity: inProximity
                )
            }
        default:
            break
        }

        return nil
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
        var activeDriver: PenDriver?

        do {
            logger.logAsync("[session] connecting to \(connection.name) (\(connection.ip))…", level: "info")
            streamProcess = try SSHKeyInstaller.connectPenStream(ip: connection.ip, keyURL: keyURL, password: password, logger: logger)
            setProcess(streamProcess?.process)
            emit(.connected)

            var config = snapshotConfig()
            let mouse = MouseController(config: config)
            var currentMode = config.outputMode
            activeDriver = makeDriver(mode: currentMode, mouse: mouse, config: config)
            var parser = PenFrameParser()

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
                        activeDriver?.cleanup()
                        continue
                    }

                    config = snapshotConfig()
                    mouse.config = config

                    if config.outputMode != currentMode {
                        activeDriver?.cleanup()
                        currentMode = config.outputMode
                        activeDriver = makeDriver(mode: currentMode, mouse: mouse, config: config)
                    }

                    if let absoluteDriver = activeDriver as? AbsolutePenDriver {
                        absoluteDriver.updateRegion(config.absolute)
                    }
                    activeDriver?.handle(frame: frame)
                }
            }

            activeDriver?.cleanup()
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
            activeDriver?.cleanup()
            if !shouldStop() {
                logger.logAsync("[session] error: \(error.localizedDescription)", level: "error")
                emit(.failed(error.localizedDescription))
            }
        }

        setProcess(nil)
        lock.lock()
        thread = nil
        lock.unlock()
    }

    private func makeDriver(mode: OutputMode, mouse: MouseController, config: DeviceConfig) -> PenDriver {
        switch mode {
        case .absolute:
            AbsolutePenDriver(mouse: mouse, region: config.absolute)
        case .relative:
            RelativePenDriver(mouse: mouse)
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
