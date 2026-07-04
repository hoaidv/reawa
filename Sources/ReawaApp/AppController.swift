import AppKit
import Combine

@MainActor
private func setDockVisible(_ visible: Bool) {
    if visible {
        NSApp.setActivationPolicy(.regular)
        NSApp.activate(ignoringOtherApps: true)
    } else {
        NSApp.setActivationPolicy(.accessory)
    }
}

@MainActor
final class AppController: NSObject, NSApplicationDelegate {
    private let logger = AppLogger.shared
    private lazy var manager = ConnectionManager(logger: logger)
    private lazy var notifications = NotificationService(logger: logger)
    private lazy var windowSnap = WindowSnapController()
    private lazy var picker = PickerOverlayController()
    private lazy var regionOverlay = RegionOverlayController { [weak self] region in
        self?.regionChanged(region)
    }
    private lazy var settingsViewModel = SettingsViewModel(
        manager: manager,
        logger: logger
    ) { [weak self] connectionID, mode in
        self?.settingsModeChanged(connectionID: connectionID, mode: mode)
    }
    private lazy var settingsWindow = SettingsWindowController(viewModel: settingsViewModel, logger: logger)
    private lazy var usbWatcher = USBWatcher(manager: manager, notifications: notifications, logger: logger)

    private var statusItem: NSStatusItem?
    private var followTimer: Timer?
    private var cancellables: Set<AnyCancellable> = []
    private var picking = false
    private var snappedConnectionID: String?
    private var snappedWindowState: WindowLifecycle = .normal
    private var hoveredWindowReference: String?

    func applicationDidFinishLaunching(_ notification: Notification) {
        setDockVisible(false)
        configureStatusItem()
        configureSettingsWindow()
        configureObservers()
        usbWatcher.setOnDetected { [weak self] _ in
            self?.refreshMenu()
        }
        usbWatcher.start()
        refreshMenu()
        logger.log("Reawa started.", level: "info", category: .app)
    }

    func applicationWillTerminate(_ notification: Notification) {
        usbWatcher.stop()
        picker.stop()
        manager.disconnect()
        regionOverlay.hide()
        logger.log("Reawa is shutting down.", level: "info", category: .app)
    }

    private func configureStatusItem() {
        let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        if let image = NSImage(systemSymbolName: "pencil.tip.crop.circle", accessibilityDescription: "Reawa") {
            image.isTemplate = true
            item.button?.image = image
        } else {
            item.button?.title = "Reawa"
        }
        statusItem = item
    }

    private func configureSettingsWindow() {
        settingsWindow.onOpen = { [weak self] in
            setDockVisible(true)
            self?.logger.log("Settings window opened.", level: "info", category: .app)
        }
        settingsWindow.onClose = { [weak self] in
            setDockVisible(false)
            self?.logger.log("Settings window closed.", level: "info", category: .app)
        }
    }

    private func configureObservers() {
        manager.objectWillChange
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.refreshMenu()
            }
            .store(in: &cancellables)
    }

    private func activeConnection() -> Connection? {
        guard let activeConnectionID = manager.activeConnectionID else {
            return nil
        }
        return manager.connection(id: activeConnectionID)
    }

    private func regionChanged(_ region: AbsoluteConfig) {
        guard var connection = activeConnection() else {
            return
        }
        connection.deviceConfig.absolute = region
        manager.updateConnection(connection)
        windowSnap.syncWindowToRegion(region)
    }

    private func startPick(connectionID: String) {
        guard !picking else {
            return
        }
        picking = true
        hoveredWindowReference = nil
        manager.pauseInput()
        regionOverlay.hide()
        stopFollowTimer()
        settingsViewModel.statusMessage = "Selecting a window…"
        let label = manager.connection(id: connectionID)?.name ?? connectionID
        logger.log("Starting window picker for \(label).", level: "info", category: .absolute)
        picker.start(
            onHoverChange: { [weak self] info in
                self?.hoverWindowChanged(info)
            },
            onPick: { [weak self] info in
                self?.picked(info)
            },
            onCancel: { [weak self] in
                self?.pickCancelled()
            }
        )
    }

    private func cancelPick() {
        if picking {
            picker.stop()
            picking = false
            hoveredWindowReference = nil
        }
    }

    private func picked(_ info: WindowInfo) {
        guard var connection = activeConnection() else {
            picking = false
            return
        }

        let (reference, bounds) = windowSnap.pick(from: info)
        let windowFrame = windowSnap.restoreWindow() ?? bounds
        var region = connection.deviceConfig.absolute
        windowSnap.snapRegionToWindow(&region, windowFrame: windowFrame)
        windowSnap.syncWindowToRegion(region)
        region.snapWindowEnabled = true
        region.snappedWindowRef = reference
        connection.deviceConfig.absolute = region
        connection.deviceConfig.outputMode = .absolute
        manager.updateConnection(connection)

        picking = false
        snappedConnectionID = connection.id
        snappedWindowState = .normal
        manager.resumeInput()
        settingsViewModel.selectConnection(connection)
        settingsViewModel.statusMessage = "Snapped to \(reference)"
        logger.log("Chose window \(reference) for Absolute mode.", level: "info", category: .absolute)
        refreshMenu()
    }

    private func pickCancelled() {
        picking = false
        guard let connection = activeConnection() else {
            return
        }
        logger.log("Window picker cancelled for \(connection.name). Reverting to Relative mode.", level: "info", category: .absolute)
        revertToRelative(connectionID: connection.id)
    }

    private func switchToRelative() {
        switchToMode(.relative)
    }

    private func switchToAbsolute() {
        switchToMode(.absolute)
    }

    private func switchToNativeStylus() {
        switchToMode(.nativeStylus)
    }

    private func switchToMode(_ mode: OutputMode) {
        guard var connection = activeConnection() else {
            return
        }
        logger.log("Switching \(connection.name) to \(mode.title) mode.", level: "info", category: .mode)

        switch mode {
        case .relative:
            revertToRelative(connectionID: connection.id)
        case .absolute:
            connection.deviceConfig.outputMode = .absolute
            manager.updateConnection(connection)
            snappedConnectionID = nil
            cancelPick()
            refreshMenu()
        case .nativeStylus:
            cancelPick()
            snappedConnectionID = nil
            stopFollowTimer()
            regionOverlay.hide()
            manager.resumeInput()
            connection.deviceConfig.outputMode = .nativeStylus
            manager.updateConnection(connection)
            settingsViewModel.statusMessage = "Native Stylus mode"
            refreshMenu()
        }
    }

    private func restartPick() {
        guard let connection = activeConnection() else {
            return
        }
        cancelPick()
        snappedConnectionID = nil
        logger.log("Restarting window picker for \(connection.name).", level: "info", category: .absolute)
        startPick(connectionID: connection.id)
    }

    private func revertToRelative(connectionID: String) {
        cancelPick()
        guard var connection = manager.connection(id: connectionID) else {
            return
        }
        connection.deviceConfig.outputMode = .relative
        connection.deviceConfig.absolute.snapWindowEnabled = false
        connection.deviceConfig.absolute.snappedWindowRef = nil
        windowSnap.clear()
        snappedConnectionID = nil
        snappedWindowState = .normal
        manager.updateConnection(connection)
        regionOverlay.hide()
        stopFollowTimer()
        manager.resumeInput()
        settingsViewModel.selectConnection(connection)
        settingsViewModel.statusMessage = "Relative mode"
        logger.log("Relative mode active for \(connection.name).", level: "info", category: .mode)
        refreshMenu()
    }

    private func stopFollowTimer() {
        followTimer?.invalidate()
        followTimer = nil
    }

    @objc private func followTick() {
        guard !picking,
              let snappedConnectionID,
              let connection = activeConnection(),
              snappedConnectionID == connection.id,
              connection.deviceConfig.outputMode == .absolute
        else {
            return
        }

        let lifecycle = windowSnap.snappedLifecycleState()
        var region = connection.deviceConfig.absolute

        switch lifecycle {
        case .closed:
            if snappedWindowState != .closed {
                logger.log("Snapped window closed. Returning to Relative mode.", level: "info", category: .absolute)
                snappedWindowState = .closed
            }
            revertToRelative(connectionID: connection.id)
            return
        case .minimized:
            if snappedWindowState != .minimized {
                regionOverlay.hide()
                snappedWindowState = .minimized
                logger.log("Snapped window minimized. Overlay hidden.", level: "info", category: .absolute)
            }
            return
        case .maximized:
            if let frame = windowSnap.currentWindowFrame() {
                windowSnap.snapRegionToWindow(&region, windowFrame: frame)
                windowSnap.syncWindowToRegion(region)
                applyRegion(region, to: connection)
                regionOverlay.show(region)
            }
            if snappedWindowState != .maximized {
                snappedWindowState = .maximized
                logger.log("Snapped window maximized. Region resynced.", level: "info", category: .absolute)
            }
            return
        case .normal:
            break
        }

        if snappedWindowState == .minimized || snappedWindowState == .maximized {
            snappedWindowState = .normal
            if windowSnap.syncRegionToWindow(&region) {
                applyRegion(region, to: connection)
            }
            regionOverlay.show(region)
            logger.log("Snapped window returned to normal state.", level: "info", category: .absolute)
        }

        guard let frame = windowSnap.currentWindowFrame() else {
            return
        }
        let moved = abs(frame.minX - region.regionX) > 2 || abs(frame.minY - region.regionY) > 2
        let resized = abs(frame.width - region.regionWidth) > 2 || abs(frame.height - region.regionHeight) > 2
        if moved || resized {
            if resized {
                windowSnap.snapRegionToWindow(&region, windowFrame: frame)
                windowSnap.syncWindowToRegion(region)
            } else {
                region.regionX = frame.minX
                region.regionY = frame.minY
            }
            regionOverlay.updateRegion(region)
            applyRegion(region, to: connection)
        }
    }

    private func applyRegion(_ region: AbsoluteConfig, to connection: Connection) {
        var updated = connection
        updated.deviceConfig.absolute = region
        manager.updateConnection(updated)
        settingsViewModel.selectConnection(updated)
    }

    private func refreshMenu() {
        let menu = NSMenu()
        menu.autoenablesItems = false

        for connection in manager.connections {
            let item = NSMenuItem(
                title: "\(statusPrefix(manager.status(for: connection.id))) \(connection.name)",
                action: #selector(toggleConnectionFromMenu(_:)),
                keyEquivalent: ""
            )
            item.target = self
            item.representedObject = connection.id
            menu.addItem(item)
        }

        menu.addItem(.separator())
        addModeItems(to: menu)
        menu.addItem(.separator())

        menu.addItem(makeMenuItem("Open", action: #selector(openSettings)))
        menu.addItem(makeMenuItem("About Reawa", action: #selector(showAbout)))
        menu.addItem(makeMenuItem("Quit", action: #selector(quitApp)))

        statusItem?.menu = menu
        updateOverlay()
    }

    private func addModeItems(to menu: NSMenu) {
        let mode = activeMode()
        for candidate in OutputMode.allCases {
            let item = makeMenuItem(
                "\(mode == candidate ? "🟢" : "⚪️") \(candidate.title)",
                action: mode == nil || mode == candidate ? nil : selector(for: candidate)
            )
            item.isEnabled = mode != nil && mode != candidate
            menu.addItem(item)
        }

        if mode == .absolute {
            menu.addItem(.separator())
            let label = makeMenuItem(sendingToLabel(activeConnection()?.deviceConfig.absolute.snappedWindowRef), action: nil)
            label.isEnabled = false
            menu.addItem(label)
            menu.addItem(makeMenuItem("Choose window", action: #selector(menuChooseWindow)))
        }
    }

    private func activeMode() -> OutputMode? {
        guard let connection = activeConnection(),
              manager.status(for: connection.id) == .connected
        else {
            return nil
        }
        return connection.deviceConfig.outputMode
    }

    private func sendingToLabel(_ reference: String?) -> String {
        guard let reference, !reference.isEmpty else {
            return "Sending to …"
        }
        let truncated = reference.count > 28 ? String(reference.prefix(27)) + "…" : reference
        return "Sending to \(truncated)"
    }

    private func updateOverlay() {
        guard let connection = activeConnection(),
              manager.status(for: connection.id) == .connected
        else {
            if !picking {
                regionOverlay.hide()
                stopFollowTimer()
                snappedConnectionID = nil
            }
            return
        }

        guard connection.deviceConfig.outputMode == .absolute else {
            regionOverlay.hide()
            stopFollowTimer()
            snappedConnectionID = nil
            return
        }

        if picking {
            return
        }

        if snappedConnectionID != connection.id {
            startPick(connectionID: connection.id)
            return
        }

        if snappedWindowState != .minimized && windowSnap.snappedLifecycleState() != .minimized {
            regionOverlay.show(connection.deviceConfig.absolute)
        }

        if followTimer == nil {
            followTimer = Timer.scheduledTimer(timeInterval: 0.4, target: self, selector: #selector(followTick), userInfo: nil, repeats: true)
        }
    }

    private func settingsModeChanged(connectionID: String, mode: OutputMode) {
        switch mode {
        case .relative:
            if manager.activeConnectionID == connectionID {
                revertToRelative(connectionID: connectionID)
            } else if var connection = manager.connection(id: connectionID) {
                connection.deviceConfig.absolute.snapWindowEnabled = false
                connection.deviceConfig.absolute.snappedWindowRef = nil
                manager.updateConnection(connection)
                refreshMenu()
            }
        case .absolute:
            if manager.activeConnectionID == connectionID {
                snappedConnectionID = nil
                cancelPick()
            }
            refreshMenu()
        case .nativeStylus:
            if manager.activeConnectionID == connectionID {
                snappedConnectionID = nil
                cancelPick()
                stopFollowTimer()
                regionOverlay.hide()
                manager.resumeInput()
            }
            refreshMenu()
        }
    }

    private func hoverWindowChanged(_ info: WindowInfo?) {
        guard let info else {
            hoveredWindowReference = nil
            return
        }
        let reference = windowReference(for: info)
        guard hoveredWindowReference != reference else {
            return
        }
        hoveredWindowReference = reference
        logger.log("Hovering window \(reference) in picker.", level: "info", category: .absolute)
    }

    private func windowReference(for info: WindowInfo) -> String {
        info.name.isEmpty ? "pid \(info.pid)" : info.name
    }

    private func makeMenuItem(_ title: String, action: Selector?) -> NSMenuItem {
        let item = NSMenuItem(title: title, action: action, keyEquivalent: "")
        item.target = self
        return item
    }

    private func statusPrefix(_ status: ConnectionStatus) -> String {
        switch status {
        case .offline: return "○"
        case .online: return "◎"
        case .connected: return "●"
        case .error: return "✗"
        }
    }

    @objc private func toggleConnectionFromMenu(_ sender: NSMenuItem) {
        guard let connectionID = sender.representedObject as? String else {
            return
        }
        do {
            try manager.toggleConnection(connectionID)
        } catch {
            logger.log(error.localizedDescription, level: "error", category: .connection)
        }
        refreshMenu()
    }

    @objc private func menuSwitchRelative() {
        switchToRelative()
    }

    @objc private func menuSwitchAbsolute() {
        switchToAbsolute()
    }

    @objc private func menuSwitchNativeStylus() {
        switchToNativeStylus()
    }

    @objc private func menuChooseWindow() {
        restartPick()
    }

    @objc private func openSettings() {
        settingsWindow.show()
    }

    @objc private func showAbout() {
        let alert = NSAlert()
        alert.messageText = "About Reawa"
        alert.informativeText = """
        Reawa turns a reMarkable tablet into a native macOS pen input device.

        License: MIT

        reMarkable is a registered trademark of reMarkable AS. Reawa is an independent project and is not affiliated with, endorsed by, or sponsored by reMarkable AS.
        """
        alert.runModal()
    }

    @objc private func quitApp() {
        NSApp.terminate(nil)
    }

    private func selector(for mode: OutputMode) -> Selector {
        switch mode {
        case .relative:
            return #selector(menuSwitchRelative)
        case .absolute:
            return #selector(menuSwitchAbsolute)
        case .nativeStylus:
            return #selector(menuSwitchNativeStylus)
        }
    }
}
