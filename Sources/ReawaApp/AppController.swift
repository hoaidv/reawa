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
    private lazy var usbWatcher = USBWatcher(manager: manager, notifications: notifications)

    private var statusItem: NSStatusItem?
    private var followTimer: Timer?
    private var cancellables: Set<AnyCancellable> = []
    private var picking = false
    private var snappedConnectionID: String?
    private var snappedWindowState: WindowLifecycle = .normal

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
        logger.log("Reawa started", level: "info")
    }

    func applicationWillTerminate(_ notification: Notification) {
        usbWatcher.stop()
        picker.stop()
        manager.disconnect()
        regionOverlay.hide()
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
        settingsWindow.onOpen = {
            setDockVisible(true)
        }
        settingsWindow.onClose = {
            setDockVisible(false)
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
        manager.pauseInput()
        regionOverlay.hide()
        stopFollowTimer()
        settingsViewModel.statusMessage = "Selecting a window…"
        picker.start(
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
        refreshMenu()
    }

    private func pickCancelled() {
        picking = false
        guard let connection = activeConnection() else {
            return
        }
        revertToRelative(connectionID: connection.id)
    }

    private func switchToRelative() {
        guard let connection = activeConnection() else {
            return
        }
        revertToRelative(connectionID: connection.id)
    }

    private func switchToAbsolute() {
        guard var connection = activeConnection() else {
            return
        }
        connection.deviceConfig.outputMode = .absolute
        manager.updateConnection(connection)
        snappedConnectionID = nil
        cancelPick()
        refreshMenu()
    }

    private func restartPick() {
        guard let connection = activeConnection() else {
            return
        }
        cancelPick()
        snappedConnectionID = nil
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
            revertToRelative(connectionID: connection.id)
            return
        case .minimized:
            if snappedWindowState != .minimized {
                regionOverlay.hide()
                snappedWindowState = .minimized
            }
            return
        case .maximized:
            if let frame = windowSnap.currentWindowFrame() {
                windowSnap.snapRegionToWindow(&region, windowFrame: frame)
                windowSnap.syncWindowToRegion(region)
                applyRegion(region, to: connection)
                regionOverlay.show(region)
            }
            return
        case .normal:
            break
        }

        if snappedWindowState == .minimized {
            snappedWindowState = .normal
            if windowSnap.syncRegionToWindow(&region) {
                applyRegion(region, to: connection)
            }
            regionOverlay.show(region)
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
        let relative = makeMenuItem(
            "\(mode == .relative ? "🟢" : "⚪️") Relative",
            action: mode == .absolute ? #selector(menuSwitchRelative) : nil
        )
        let absolute = makeMenuItem(
            "\(mode == .absolute ? "🟢" : "⚪️") Absolute",
            action: mode == .relative ? #selector(menuSwitchAbsolute) : nil
        )

        relative.isEnabled = mode == .absolute
        absolute.isEnabled = mode == .relative

        menu.addItem(relative)
        menu.addItem(absolute)

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
        if mode == .relative {
            if manager.activeConnectionID == connectionID {
                revertToRelative(connectionID: connectionID)
            } else if var connection = manager.connection(id: connectionID) {
                connection.deviceConfig.absolute.snapWindowEnabled = false
                connection.deviceConfig.absolute.snappedWindowRef = nil
                manager.updateConnection(connection)
                refreshMenu()
            }
            return
        }

        if manager.activeConnectionID == connectionID {
            snappedConnectionID = nil
            cancelPick()
        }
        refreshMenu()
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
            logger.log(error.localizedDescription, level: "error")
        }
        refreshMenu()
    }

    @objc private func menuSwitchRelative() {
        switchToRelative()
    }

    @objc private func menuSwitchAbsolute() {
        switchToAbsolute()
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
}
