import AppKit
import Combine
import SwiftUI

enum TabletOrientation: String, CaseIterable, Identifiable {
    case gutOnTop
    case gutToLeft
    case gutAtBottom
    case gutToRight

    var id: Self { self }

    var title: String {
        switch self {
        case .gutOnTop:
            return "Gut on top"
        case .gutToLeft:
            return "Gut to the left"
        case .gutAtBottom:
            return "Gut at bottom"
        case .gutToRight:
            return "Gut to the right"
        }
    }

    var swapXY: Bool {
        switch self {
        case .gutOnTop, .gutAtBottom:
            return false
        case .gutToLeft, .gutToRight:
            return true
        }
    }

    var invertX: Bool {
        switch self {
        case .gutAtBottom, .gutToRight:
            return true
        case .gutOnTop, .gutToLeft:
            return false
        }
    }

    var invertY: Bool {
        switch self {
        case .gutToLeft, .gutAtBottom:
            return true
        case .gutOnTop, .gutToRight:
            return false
        }
    }

    init(swapXY: Bool, invertX: Bool, invertY: Bool) {
        switch (swapXY, invertX, invertY) {
        case (false, false, false):
            self = .gutOnTop
        case (true, false, true):
            self = .gutToLeft
        case (false, true, true):
            self = .gutAtBottom
        case (true, true, false):
            self = .gutToRight
        default:
            self = .gutOnTop
        }
    }
}

struct ConnectionDraft: Equatable {
    var name = ""
    var ip = ""
    var password = ""
    var autoConnect = false
    var outputMode: OutputMode = .relative
    var scaleText = ""
    var swapXY = false
    var invertX = false
    var invertY = false
    var borderColor = AbsoluteConfig.defaultBorderColor
    var snappedWindowReference: String? = nil

    var trimmedName: String { name.trimmingCharacters(in: .whitespacesAndNewlines) }
    var trimmedIP: String { ip.trimmingCharacters(in: .whitespacesAndNewlines) }
    var trimmedPassword: String { password.trimmingCharacters(in: .whitespacesAndNewlines) }
    var parsedScale: Double? { Double(scaleText.trimmingCharacters(in: .whitespacesAndNewlines)) }
    var tabletOrientation: TabletOrientation {
        get { TabletOrientation(swapXY: swapXY, invertX: invertX, invertY: invertY) }
        set {
            swapXY = newValue.swapXY
            invertX = newValue.invertX
            invertY = newValue.invertY
        }
    }

    mutating func load(from connection: Connection) {
        name = connection.name
        ip = connection.ip
        password = ""
        autoConnect = connection.autoConnect
        outputMode = connection.deviceConfig.outputMode
        scaleText = connection.deviceConfig.scale.map { String($0) } ?? ""
        swapXY = connection.deviceConfig.swapXY
        invertX = connection.deviceConfig.invertX
        invertY = connection.deviceConfig.invertY
        borderColor = connection.deviceConfig.absolute.borderColor
        snappedWindowReference = connection.deviceConfig.absolute.snappedWindowRef
    }

    func applied(to connection: Connection) -> Connection {
        var updated = connection
        updated.name = trimmedName.isEmpty ? connection.name : trimmedName
        updated.ip = trimmedIP.isEmpty ? connection.ip : trimmedIP
        updated.autoConnect = autoConnect
        updated.deviceConfig.outputMode = outputMode
        updated.deviceConfig.scale = parsedScale
        updated.deviceConfig.swapXY = swapXY
        updated.deviceConfig.invertX = invertX
        updated.deviceConfig.invertY = invertY
        updated.deviceConfig.absolute.borderColor = borderColor.isEmpty ? AbsoluteConfig.defaultBorderColor : borderColor
        updated.deviceConfig.absolute.snappedWindowRef = snappedWindowReference
        return updated
    }
}

@MainActor
final class SettingsViewModel: ObservableObject {
    @Published var draft = ConnectionDraft()
    @Published var selectedConnectionID: String?
    @Published var isSaving = false
    @Published var isScanning = false
    @Published var statusMessage = ""

    let manager: ConnectionManager
    let logger: AppLogger
    private var cancellables: Set<AnyCancellable> = []
    private let onModeChanged: (String, OutputMode) -> Void

    init(
        manager: ConnectionManager,
        logger: AppLogger,
        onModeChanged: @escaping (String, OutputMode) -> Void
    ) {
        self.manager = manager
        self.logger = logger
        self.onModeChanged = onModeChanged

        manager.objectWillChange
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.syncSelection()
            }
            .store(in: &cancellables)
    }

    var connections: [Connection] {
        manager.connections
    }

    var filteredDiscoveredIPs: [String] {
        let saved = Set(manager.connections.map(\.ip))
        return manager.discoveredIPs.filter { !saved.contains($0) }
    }

    var editingConnection: Connection? {
        guard let selectedConnectionID else {
            return nil
        }
        return manager.connection(id: selectedConnectionID)
    }

    var selectedConnectionStatus: ConnectionStatus? {
        guard let selectedConnectionID else {
            return nil
        }
        return manager.status(for: selectedConnectionID)
    }

    var canSave: Bool {
        if editingConnection == nil {
            return !draft.trimmedName.isEmpty && !draft.trimmedIP.isEmpty && !draft.trimmedPassword.isEmpty
        }
        return !draft.trimmedName.isEmpty && !draft.trimmedIP.isEmpty
    }

    func beginNewConnection(prefilledIP: String? = nil) {
        selectedConnectionID = nil
        draft = ConnectionDraft(name: "reMarkable", ip: prefilledIP ?? "", password: "", autoConnect: false, outputMode: .relative, scaleText: "", swapXY: false, invertX: false, invertY: false, borderColor: AbsoluteConfig.defaultBorderColor, snappedWindowReference: nil)
        statusMessage = "New connection"
    }

    func selectConnection(_ connection: Connection) {
        selectedConnectionID = connection.id
        var next = ConnectionDraft()
        next.load(from: connection)
        draft = next
        statusMessage = ""
    }

    func selectDiscoveredIP(_ ip: String) {
        beginNewConnection(prefilledIP: ip)
    }

    func scanDevices() {
        guard !isScanning else {
            return
        }

        isScanning = true
        Task.detached(priority: .utility) {
            let discovered = NetworkDiscovery.discoverUSBSSHHosts().sorted()
            await MainActor.run {
                self.manager.setDiscoveredIPs(Set(discovered))
                self.isScanning = false
            }
        }
    }

    func save() {
        guard canSave else {
            return
        }

        if editingConnection != nil {
            applyDraftChangesIfNeeded()
            return
        }

        isSaving = true
        statusMessage = ""

        let name = draft.trimmedName.isEmpty ? "reMarkable" : draft.trimmedName
        let ip = draft.trimmedIP.isEmpty ? "10.11.99.1" : draft.trimmedIP
        let password = draft.trimmedPassword
        let autoConnect = draft.autoConnect
        let initialDraft = draft

        Task {
            do {
                var connection = try await manager.addConnection(name: name, ip: ip, password: password, autoConnect: autoConnect)
                connection = initialDraft.applied(to: connection)
                manager.updateConnection(connection)
                selectConnection(connection)
                statusMessage = "Added \(connection.name)"
            } catch {
                statusMessage = error.localizedDescription
            }
            isSaving = false
        }
    }

    func applyDraftChangesIfNeeded() {
        guard let connection = editingConnection else {
            return
        }
        guard !draft.trimmedName.isEmpty, !draft.trimmedIP.isEmpty else {
            return
        }

        let updated = draft.applied(to: connection)
        guard updated != connection else {
            return
        }

        manager.updateConnection(updated)
    }

    func removeSelectedConnection() {
        guard let selectedConnectionID else {
            return
        }
        manager.removeConnection(selectedConnectionID)
        beginNewConnection()
    }

    func toggleSelectedConnection() {
        guard let selectedConnectionID else {
            return
        }
        do {
            try manager.toggleConnection(selectedConnectionID)
        } catch {
            statusMessage = error.localizedDescription
        }
    }

    func syncSelection() {
        guard let selectedConnectionID,
              let connection = manager.connection(id: selectedConnectionID)
        else {
            return
        }
        draft.outputMode = connection.deviceConfig.outputMode
        draft.snappedWindowReference = connection.deviceConfig.absolute.snappedWindowRef
    }

    func notifyModeChanged(_ mode: OutputMode) {
        guard let connection = editingConnection else {
            return
        }
        guard connection.deviceConfig.outputMode != mode else {
            return
        }

        var updated = connection
        updated.deviceConfig.outputMode = mode
        manager.updateConnection(updated)
        draft.outputMode = mode
        draft.snappedWindowReference = updated.deviceConfig.absolute.snappedWindowRef
        onModeChanged(updated.id, mode)
    }
}

struct SettingsRootView: View {
    @ObservedObject var viewModel: SettingsViewModel
    @ObservedObject var logger: AppLogger

    var body: some View {
        TabView {
            connectionsTab
                .tabItem { Text("Connections") }
            logsTab
                .tabItem { Text("Logs") }
        }
        .padding(16)
        .frame(minWidth: 860, minHeight: 560)
        .onAppear {
            if viewModel.connections.isEmpty {
                viewModel.beginNewConnection()
            } else if let first = viewModel.connections.first {
                viewModel.selectConnection(first)
            }
            viewModel.scanDevices()
        }
    }

    private var connectionsTab: some View {
        HStack(alignment: .top, spacing: 16) {
            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    Text("Discovered")
                        .font(.headline)
                    Spacer()
                    if viewModel.isScanning {
                        ProgressView()
                            .controlSize(.small)
                    }
                    Button("Scan devices") {
                        viewModel.scanDevices()
                    }
                }

                List(viewModel.filteredDiscoveredIPs, id: \.self) { ip in
                    Button(ip) {
                        viewModel.selectDiscoveredIP(ip)
                    }
                    .buttonStyle(.plain)
                }
                .frame(minWidth: 260, minHeight: 120, maxHeight: 150)

                Text("Connections")
                    .font(.headline)

                List(viewModel.connections, id: \.id, selection: $viewModel.selectedConnectionID) { connection in
                    VStack(alignment: .leading, spacing: 4) {
                        Text("\(prefix(for: viewModel.manager.status(for: connection.id))) \(connection.name)")
                        Text("\(connection.ip) — \(viewModel.manager.status(for: connection.id).rawValue)")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    .tag(connection.id)
                    .onTapGesture {
                        viewModel.selectConnection(connection)
                    }
                }
                .frame(minWidth: 260, minHeight: 180)

                HStack {
                    Button("New") {
                        viewModel.beginNewConnection()
                    }
                    Button("Remove") {
                        viewModel.removeSelectedConnection()
                    }
                    .disabled(viewModel.selectedConnectionID == nil)
                    Button(connectButtonTitle) {
                        viewModel.toggleSelectedConnection()
                    }
                    .disabled(viewModel.selectedConnectionID == nil)
                }
            }
            .frame(width: 300)

            Divider()

            Form {
                Section(viewModel.editingConnection == nil ? "New connection" : "Editing \(viewModel.draft.name)") {
                    TextField("Name", text: $viewModel.draft.name)
                    TextField("IP", text: $viewModel.draft.ip)
                    SecureField("Password (new connections only)", text: $viewModel.draft.password)
                    Toggle("Auto-connect on USB detect", isOn: $viewModel.draft.autoConnect)
                    Picker("Output mode", selection: $viewModel.draft.outputMode) {
                        Text("Relative").tag(OutputMode.relative)
                        Text("Absolute").tag(OutputMode.absolute)
                    }
                    .pickerStyle(.segmented)
                    .onChange(of: viewModel.draft.outputMode) { newValue in
                        viewModel.notifyModeChanged(newValue)
                    }

                    TextField("Scale (empty = auto)", text: $viewModel.draft.scaleText)
                    TabletOrientationPicker(
                        selection: Binding(
                            get: { viewModel.draft.tabletOrientation },
                            set: { viewModel.draft.tabletOrientation = $0 }
                        )
                    )
                    TextField("Border color", text: $viewModel.draft.borderColor)
                    if viewModel.draft.outputMode == .absolute {
                        Text("Snapped window: \(viewModel.draft.snappedWindowReference ?? "(none — pick a window)")")
                            .foregroundStyle(.secondary)
                    }
                }

                Section {
                    if viewModel.editingConnection == nil {
                        Button("Add connection") {
                            viewModel.save()
                        }
                        .disabled(!viewModel.canSave || viewModel.isSaving)
                    } else {
                        Text("Changes apply automatically.")
                            .font(.callout)
                            .foregroundStyle(.secondary)
                    }

                    if !viewModel.statusMessage.isEmpty {
                        Text(viewModel.statusMessage)
                            .font(.callout)
                            .foregroundStyle(.secondary)
                    }
                }
            }
            .formStyle(.grouped)
            .onChange(of: viewModel.draft) { _ in
                viewModel.applyDraftChangesIfNeeded()
            }
        }
    }

    private var logsTab: some View {
        LogView(logger: logger)
    }

    private var connectButtonTitle: String {
        guard let selected = viewModel.selectedConnectionID else {
            return "Connect"
        }
        return viewModel.manager.status(for: selected) == .connected ? "Disconnect" : "Connect"
    }

    private func prefix(for status: ConnectionStatus) -> String {
        switch status {
        case .offline: return "○"
        case .online: return "◎"
        case .connected: return "●"
        case .error: return "✗"
        }
    }
}

struct TabletOrientationPicker: View {
    @Binding var selection: TabletOrientation

    private let columns = [
        GridItem(.flexible(), spacing: 8),
        GridItem(.flexible(), spacing: 8)
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Tablet orientation")
                .font(.headline)

            LazyVGrid(columns: columns, spacing: 8) {
                ForEach(TabletOrientation.allCases) { orientation in
                    orientationButton(orientation)
                }
            }

            Text("Adjusts pen movement to feel natural for the way the tablet is rotated.")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding(.vertical, 4)
    }

    private func orientationButton(_ orientation: TabletOrientation) -> some View {
        let isSelected = selection == orientation

        return Button {
            selection = orientation
        } label: {
            HStack(spacing: 10) {
                TabletOrientationIcon(orientation: orientation)
                VStack(alignment: .leading, spacing: 2) {
                    Text(orientation.title)
                        .font(.body)
                    if orientation == .gutOnTop {
                        Text("Default")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }
                Spacer(minLength: 0)
            }
            .padding(10)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(
                RoundedRectangle(cornerRadius: 10)
                    .fill(isSelected ? Color.accentColor.opacity(0.12) : Color(NSColor.controlBackgroundColor))
            )
            .overlay(
                RoundedRectangle(cornerRadius: 10)
                    .stroke(isSelected ? Color.accentColor : Color.secondary.opacity(0.25), lineWidth: isSelected ? 2 : 1)
            )
        }
        .buttonStyle(.plain)
    }
}

struct TabletOrientationIcon: View {
    let orientation: TabletOrientation

    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.primary.opacity(0.8), lineWidth: 1.5)
                .frame(width: 30, height: 42)

            Capsule()
                .fill(Color.accentColor)
                .frame(width: gutWidth, height: gutHeight)
                .offset(gutOffset)
        }
        .frame(width: 36, height: 48)
        .accessibilityHidden(true)
    }

    private var gutWidth: CGFloat {
        switch orientation {
        case .gutOnTop, .gutAtBottom:
            return 14
        case .gutToLeft, .gutToRight:
            return 4
        }
    }

    private var gutHeight: CGFloat {
        switch orientation {
        case .gutOnTop, .gutAtBottom:
            return 4
        case .gutToLeft, .gutToRight:
            return 14
        }
    }

    private var gutOffset: CGSize {
        switch orientation {
        case .gutOnTop:
            return CGSize(width: 0, height: -16)
        case .gutToLeft:
            return CGSize(width: -10, height: 0)
        case .gutAtBottom:
            return CGSize(width: 0, height: 16)
        case .gutToRight:
            return CGSize(width: 10, height: 0)
        }
    }
}

struct LogView: View {
    @ObservedObject var logger: AppLogger
    @State private var query = ""

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            TextField("Search logs", text: $query)
                .textFieldStyle(.roundedBorder)

            ScrollView {
                LazyVStack(alignment: .leading, spacing: 4) {
                    ForEach(filteredEntries) { entry in
                        Text(entry.formatted)
                            .font(.system(.body, design: .monospaced))
                            .frame(maxWidth: .infinity, alignment: .leading)
                    }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }
        }
    }

    private var filteredEntries: [LogEntry] {
        if query.isEmpty {
            return logger.entries
        }
        return logger.entries.filter { $0.formatted.localizedCaseInsensitiveContains(query) }
    }
}

@MainActor
final class SettingsWindowController: NSWindowController, NSWindowDelegate {
    let viewModel: SettingsViewModel
    var onOpen: (() -> Void)?
    var onClose: (() -> Void)?

    init(viewModel: SettingsViewModel, logger: AppLogger) {
        self.viewModel = viewModel
        let rootView = SettingsRootView(viewModel: viewModel, logger: logger)
        let hostingController = NSHostingController(rootView: rootView)
        let window = NSWindow(contentViewController: hostingController)
        window.title = "Reawa Settings"
        window.setContentSize(NSSize(width: 900, height: 620))
        window.isReleasedWhenClosed = false
        super.init(window: window)
        window.delegate = self
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func show() {
        showWindow(nil)
        window?.center()
        NSApp.activate(ignoringOtherApps: true)
        onOpen?()
    }

    func windowWillClose(_ notification: Notification) {
        onClose?()
    }
}
