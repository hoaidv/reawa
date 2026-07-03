import AppKit
import Combine
import SwiftUI

private enum SettingsPalette {
    static let canvas = Color(nsColor: .windowBackgroundColor)
    static let panel = Color(nsColor: .windowBackgroundColor)
    static let panelBorder = Color.primary.opacity(0.08)
    static let subtleBorder = Color.primary.opacity(0.05)
    static let selectedFill = Color.accentColor.opacity(0.12)
    static let selectedStroke = Color.accentColor.opacity(0.85)
}

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

    var isLandscape: Bool {
        switch self {
        case .gutOnTop, .gutAtBottom:
            return true
        case .gutToLeft, .gutToRight:
            return false
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
        draft = ConnectionDraft(
            name: "reMarkable",
            ip: prefilledIP ?? "",
            password: "",
            autoConnect: false,
            outputMode: .relative,
            scaleText: "",
            swapXY: false,
            invertX: false,
            invertY: false,
            borderColor: AbsoluteConfig.defaultBorderColor,
            snappedWindowReference: nil
        )
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
        applyConnectionUpdate(updated, original: connection, shouldNotifyModeChange: false)
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
        draft.borderColor = connection.deviceConfig.absolute.borderColor
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
        applyConnectionUpdate(updated, original: connection, shouldNotifyModeChange: true)
    }

    private func applyConnectionUpdate(
        _ updated: Connection,
        original connection: Connection,
        shouldNotifyModeChange: Bool
    ) {
        guard updated != connection else {
            return
        }

        let changes = settingsChanges(from: connection, to: updated)
        manager.updateConnection(updated)

        if !changes.isEmpty {
            logger.log(
                "\(updated.name): \(changes.joined(separator: "; "))",
                level: "info",
                category: .settings
            )
        }

        if shouldNotifyModeChange, updated.deviceConfig.outputMode != connection.deviceConfig.outputMode {
            draft.outputMode = updated.deviceConfig.outputMode
            draft.snappedWindowReference = updated.deviceConfig.absolute.snappedWindowRef
            onModeChanged(updated.id, updated.deviceConfig.outputMode)
        }
    }

    private func settingsChanges(from old: Connection, to new: Connection) -> [String] {
        var changes: [String] = []

        if old.name != new.name {
            changes.append("name -> \(new.name)")
        }
        if old.ip != new.ip {
            changes.append("IP -> \(new.ip)")
        }
        if old.autoConnect != new.autoConnect {
            changes.append("auto-connect -> \(new.autoConnect ? "on" : "off")")
        }
        if old.deviceConfig.outputMode != new.deviceConfig.outputMode {
            changes.append("output mode -> \(new.deviceConfig.outputMode == .relative ? "Relative" : "Absolute")")
        }
        if old.deviceConfig.scale != new.deviceConfig.scale {
            if let scale = new.deviceConfig.scale {
                changes.append("scale -> \(scale)")
            } else {
                changes.append("scale -> Auto")
            }
        }

        let oldOrientation = TabletOrientation(
            swapXY: old.deviceConfig.swapXY,
            invertX: old.deviceConfig.invertX,
            invertY: old.deviceConfig.invertY
        )
        let newOrientation = TabletOrientation(
            swapXY: new.deviceConfig.swapXY,
            invertX: new.deviceConfig.invertX,
            invertY: new.deviceConfig.invertY
        )
        if oldOrientation != newOrientation {
            changes.append("tablet orientation -> \(newOrientation.title)")
        }

        if old.deviceConfig.absolute.borderColor != new.deviceConfig.absolute.borderColor {
            changes.append("border color -> \(new.deviceConfig.absolute.borderColor)")
        }

        return changes
    }
}

struct SettingsRootView: View {
    @ObservedObject var viewModel: SettingsViewModel
    @ObservedObject var logger: AppLogger

    var body: some View {
        TabView {
            connectionsTab
                .tabItem { Text("Connections") }

            AppBehaviorLogView(logger: logger)
                .tabItem { Text("App Behavior Log") }

            PenEventLogView(logger: logger)
                .tabItem { Text("Pen Event Log") }
        }
        .frame(minWidth: 1_040, minHeight: 740)
        .background(SettingsPalette.canvas.ignoresSafeArea())
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
        HStack(alignment: .top, spacing: 18) {
            sidebar
                .frame(width: 320)

            editorPane
                .frame(maxWidth: .infinity, alignment: .topLeading)
        }
        .padding(18)
    }

    private var sidebar: some View {
        CardSurface {
            VStack(alignment: .leading, spacing: 18) {
                VStack(alignment: .leading, spacing: 10) {
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

                    SidebarList {
                        if viewModel.filteredDiscoveredIPs.isEmpty {
                            EmptySidebarMessage("No new USB-discovered devices right now.")
                        } else {
                            ForEach(viewModel.filteredDiscoveredIPs, id: \.self) { ip in
                                Button {
                                    viewModel.selectDiscoveredIP(ip)
                                } label: {
                                    Text(ip)
                                        .frame(maxWidth: .infinity, alignment: .leading)
                                }
                                .buttonStyle(.plain)
                                .padding(.horizontal, 10)
                                .padding(.vertical, 8)
                                .background(
                                    RoundedRectangle(cornerRadius: 10)
                                        .fill(SettingsPalette.panel)
                                )
                            }
                        }
                    }
                    .frame(height: 150)
                }

                VStack(alignment: .leading, spacing: 10) {
                    Text("Connections")
                        .font(.headline)

                    SidebarList {
                        if viewModel.connections.isEmpty {
                            EmptySidebarMessage("No saved connections yet.")
                        } else {
                            ForEach(viewModel.connections) { connection in
                                ConnectionSidebarRow(
                                    connection: connection,
                                    status: viewModel.manager.status(for: connection.id),
                                    isSelected: viewModel.selectedConnectionID == connection.id
                                ) {
                                    viewModel.selectConnection(connection)
                                }
                            }
                        }
                    }
                    .frame(minHeight: 320)
                }

                HStack(spacing: 10) {
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
        }
    }

    private var editorPane: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                CardSurface {
                    VStack(alignment: .leading, spacing: 8) {
                        Text(viewModel.editingConnection == nil ? "New connection" : "Editing \(viewModel.draft.name)")
                            .font(.title3.weight(.semibold))

                        HStack(spacing: 8) {
                            if let status = viewModel.selectedConnectionStatus {
                                StatusPill(label: status.rawValue.capitalized, status: status)
                            } else {
                                StatusPill(label: "Not saved yet", status: nil)
                            }

                            Spacer()

                            if !viewModel.statusMessage.isEmpty {
                                Text(viewModel.statusMessage)
                                    .font(.callout)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                }

                CardSurface {
                    VStack(alignment: .leading, spacing: 18) {
                        Text("Connection settings")
                            .font(.headline)

                        Grid(alignment: .leading, horizontalSpacing: 16, verticalSpacing: 14) {
                            GridRow {
                                SettingsFieldLabel("Name")
                                TextField("reMarkable", text: $viewModel.draft.name)
                                    .textFieldStyle(.roundedBorder)
                            }

                            GridRow {
                                SettingsFieldLabel("IP")
                                TextField("10.11.99.1", text: $viewModel.draft.ip)
                                    .textFieldStyle(.roundedBorder)
                            }

                            GridRow {
                                SettingsFieldLabel("Password")
                                SecureField("New connections only", text: $viewModel.draft.password)
                                    .textFieldStyle(.roundedBorder)
                            }

                            GridRow {
                                SettingsFieldLabel("Auto-connect")
                                Toggle("Auto-connect on USB detect", isOn: $viewModel.draft.autoConnect)
                                    .toggleStyle(.switch)
                            }

                            GridRow {
                                SettingsFieldLabel("Output mode")
                                Picker("Output mode", selection: $viewModel.draft.outputMode) {
                                    Text("Relative").tag(OutputMode.relative)
                                    Text("Absolute").tag(OutputMode.absolute)
                                }
                                .pickerStyle(.segmented)
                                .frame(maxWidth: 280)
                                .onChange(of: viewModel.draft.outputMode) { newValue in
                                    viewModel.notifyModeChanged(newValue)
                                }
                            }

                            GridRow {
                                SettingsFieldLabel("Scale")
                                TextField("Empty = auto", text: $viewModel.draft.scaleText)
                                    .textFieldStyle(.roundedBorder)
                            }

                            GridRow(alignment: .top) {
                                SettingsFieldLabel("Tablet orientation")
                                    .padding(.top, 6)

                                TabletOrientationPicker(
                                    selection: Binding(
                                        get: { viewModel.draft.tabletOrientation },
                                        set: { viewModel.draft.tabletOrientation = $0 }
                                    )
                                )
                            }

                            GridRow {
                                SettingsFieldLabel("Border color")
                                TextField(AbsoluteConfig.defaultBorderColor, text: $viewModel.draft.borderColor)
                                    .textFieldStyle(.roundedBorder)
                            }

                            if viewModel.draft.outputMode == .absolute {
                                GridRow {
                                    SettingsFieldLabel("Snapped window")
                                    Text(viewModel.draft.snappedWindowReference ?? "(none — pick a window)")
                                        .foregroundStyle(.secondary)
                                        .frame(maxWidth: .infinity, alignment: .leading)
                                }
                            }
                        }
                    }
                }

                CardSurface {
                    VStack(alignment: .leading, spacing: 10) {
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
                    }
                }
            }
            .frame(maxWidth: .infinity, alignment: .topLeading)
        }
        .onChange(of: viewModel.draft) { _ in
            viewModel.applyDraftChangesIfNeeded()
        }
    }

    private var connectButtonTitle: String {
        guard let selected = viewModel.selectedConnectionID else {
            return "Connect"
        }
        return viewModel.manager.status(for: selected) == .connected ? "Disconnect" : "Connect"
    }
}

struct TabletOrientationPicker: View {
    @Binding var selection: TabletOrientation

    private let columns = [
        GridItem(.flexible(), spacing: 10),
        GridItem(.flexible(), spacing: 10),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            LazyVGrid(columns: columns, spacing: 10) {
                ForEach(TabletOrientation.allCases) { orientation in
                    orientationButton(orientation)
                }
            }

            Text("Adjusts pen movement to feel natural for the way the tablet is rotated.")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
    }

    private func orientationButton(_ orientation: TabletOrientation) -> some View {
        let isSelected = selection == orientation

        return Button {
            selection = orientation
        } label: {
            HStack(spacing: 12) {
                TabletOrientationIcon(orientation: orientation)
                VStack(alignment: .leading, spacing: 4) {
                    Text(orientation.title)
                        .font(.body.weight(.medium))
                        .foregroundStyle(.primary)
                    if orientation == .gutOnTop {
                        Text("Default")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }
                Spacer(minLength: 0)
            }
            .padding(12)
            .frame(maxWidth: .infinity, minHeight: 74, alignment: .leading)
            .background(
                RoundedRectangle(cornerRadius: 14)
                    .fill(isSelected ? SettingsPalette.selectedFill : SettingsPalette.panel)
            )
            .overlay(
                RoundedRectangle(cornerRadius: 14)
                    .stroke(isSelected ? SettingsPalette.selectedStroke : SettingsPalette.panelBorder, lineWidth: isSelected ? 2 : 1)
            )
        }
        .buttonStyle(.plain)
    }
}

struct TabletOrientationIcon: View {
    let orientation: TabletOrientation

    var body: some View {
        RoundedRectangle(cornerRadius: 8)
            .stroke(Color.primary.opacity(0.7), lineWidth: 1.5)
            .frame(width: tabletSize.width, height: tabletSize.height)
            .overlay(alignment: gutAlignment) {
                RoundedRectangle(cornerRadius: 2)
                    .fill(Color.accentColor)
                    .frame(width: gutSize.width, height: gutSize.height)
                    .padding(gutPaddingEdges, 4)
            }
            .frame(width: 70, height: 58)
            .accessibilityHidden(true)
    }

    private var tabletSize: CGSize {
        orientation.isLandscape ? CGSize(width: 56, height: 36) : CGSize(width: 36, height: 56)
    }

    private var gutAlignment: Alignment {
        switch orientation {
        case .gutOnTop:
            return .top
        case .gutToLeft:
            return .leading
        case .gutAtBottom:
            return .bottom
        case .gutToRight:
            return .trailing
        }
    }

    private var gutSize: CGSize {
        switch orientation {
        case .gutOnTop, .gutAtBottom:
            return CGSize(width: tabletSize.width - 8, height: 5)
        case .gutToLeft, .gutToRight:
            return CGSize(width: 5, height: tabletSize.height - 8)
        }
    }

    private var gutPaddingEdges: Edge.Set {
        switch orientation {
        case .gutOnTop:
            return [.top]
        case .gutToLeft:
            return [.leading]
        case .gutAtBottom:
            return [.bottom]
        case .gutToRight:
            return [.trailing]
        }
    }
}

struct AppBehaviorLogView: View {
    @ObservedObject var logger: AppLogger
    @State private var query = ""

    private var filteredEntries: [LogEntry] {
        if query.isEmpty {
            return logger.behaviorEntries
        }
        return logger.behaviorEntries.filter { $0.searchableText.localizedCaseInsensitiveContains(query) }
    }

    var body: some View {
        LogTabShell {
            HStack(spacing: 10) {
                TextField("Search app behavior logs", text: $query)
                    .textFieldStyle(.roundedBorder)
                Button("Clear") {
                    logger.clearBehaviorLog()
                }
            }

            CardSurface {
                if filteredEntries.isEmpty {
                    LogEmptyState("No behavior logs match the current filter.")
                } else {
                    ScrollView {
                        LazyVStack(alignment: .leading, spacing: 10) {
                            ForEach(filteredEntries) { entry in
                                VStack(alignment: .leading, spacing: 6) {
                                    HStack(spacing: 8) {
                                        Text(entry.timestampText)
                                            .font(.system(.caption, design: .monospaced))
                                            .foregroundStyle(.secondary)

                                        Text(entry.category.title.uppercased())
                                            .font(.caption2.weight(.semibold))
                                            .padding(.horizontal, 6)
                                            .padding(.vertical, 3)
                                            .background(
                                                Capsule()
                                                    .fill(SettingsPalette.selectedFill)
                                            )

                                        Spacer()

                                        if entry.level == "error" {
                                            Text("ERROR")
                                                .font(.caption2.weight(.semibold))
                                                .foregroundStyle(.red)
                                        }
                                    }

                                    Text(entry.message)
                                        .font(.system(.body, design: .monospaced))
                                        .foregroundStyle(entry.level == "error" ? .red : .primary)
                                        .frame(maxWidth: .infinity, alignment: .leading)
                                }
                                .padding(.bottom, 10)

                                if entry.id != filteredEntries.last?.id {
                                    Divider()
                                }
                            }
                        }
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                }
            }
        }
    }
}

struct PenEventLogView: View {
    @ObservedObject var logger: AppLogger
    @State private var query = ""

    private var captureBinding: Binding<Bool> {
        Binding(
            get: { logger.penLoggingEnabled },
            set: { logger.setPenLoggingEnabled($0) }
        )
    }

    private var filteredEntries: [PenLogEntry] {
        if query.isEmpty {
            return logger.penEntries
        }
        return logger.penEntries.filter { $0.searchableText.localizedCaseInsensitiveContains(query) }
    }

    var body: some View {
        LogTabShell {
            CardSurface {
                VStack(alignment: .leading, spacing: 14) {
                    HStack(spacing: 10) {
                        Toggle("Capture pen events", isOn: captureBinding)
                            .toggleStyle(.switch)

                        Spacer(minLength: 12)

                        TextField("Search pen events", text: $query)
                            .textFieldStyle(.roundedBorder)
                            .frame(maxWidth: 320)

                        Button("Clear") {
                            logger.clearPenLog()
                        }
                    }

                    if !logger.penCapabilityLabels.isEmpty {
                        ScrollView(.horizontal, showsIndicators: false) {
                            HStack(spacing: 8) {
                                ForEach(logger.penCapabilityLabels, id: \.self) { capability in
                                    Button {
                                        query = capability
                                    } label: {
                                        Text(capability)
                                            .font(.caption)
                                            .padding(.horizontal, 8)
                                            .padding(.vertical, 5)
                                            .background(
                                                Capsule()
                                                    .fill(SettingsPalette.selectedFill)
                                            )
                                    }
                                    .buttonStyle(.plain)
                                }
                            }
                        }
                    }
                }
            }

            CardSurface {
                if filteredEntries.isEmpty {
                    LogEmptyState(logger.penLoggingEnabled ? "No pen events captured yet." : "Pen event capture is currently off.")
                } else {
                    ScrollView {
                        LazyVStack(alignment: .leading, spacing: 8) {
                            ForEach(filteredEntries) { entry in
                                Text(entry.formatted)
                                    .font(.system(.body, design: .monospaced))
                                    .frame(maxWidth: .infinity, alignment: .leading)
                                    .padding(.bottom, 6)

                                if entry.id != filteredEntries.last?.id {
                                    Divider()
                                }
                            }
                        }
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                }
            }
        }
    }
}

struct LogTabShell<Content: View>: View {
    @ViewBuilder let content: () -> Content

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            content()
        }
        .padding(18)
        .background(SettingsPalette.canvas)
    }
}

struct CardSurface<Content: View>: View {
    @ViewBuilder let content: () -> Content

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            content()
        }
        .padding(16)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(
            RoundedRectangle(cornerRadius: 18)
                .fill(SettingsPalette.panel)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 18)
                .stroke(SettingsPalette.panelBorder, lineWidth: 1)
        )
    }
}

struct SidebarList<Content: View>: View {
    @ViewBuilder let content: () -> Content

    var body: some View {
        ScrollView {
            LazyVStack(alignment: .leading, spacing: 8) {
                content()
            }
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(10)
        .background(
            RoundedRectangle(cornerRadius: 14)
                .fill(SettingsPalette.panel)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 14)
                .stroke(SettingsPalette.subtleBorder, lineWidth: 1)
        )
    }
}

struct EmptySidebarMessage: View {
    let text: String

    init(_ text: String) {
        self.text = text
    }

    var body: some View {
        Text(text)
            .font(.callout)
            .foregroundStyle(.secondary)
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(.horizontal, 6)
            .padding(.vertical, 4)
    }
}

struct ConnectionSidebarRow: View {
    let connection: Connection
    let status: ConnectionStatus
    let isSelected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            VStack(alignment: .leading, spacing: 6) {
                HStack(spacing: 6) {
                    Circle()
                        .fill(statusColor)
                        .frame(width: 8, height: 8)
                    Text(connection.name)
                        .font(.body.weight(.medium))
                        .foregroundStyle(.primary)
                }

                Text("\(connection.ip) — \(status.rawValue)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(10)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(isSelected ? SettingsPalette.selectedFill : SettingsPalette.panel)
            )
            .overlay(
                RoundedRectangle(cornerRadius: 12)
                    .stroke(isSelected ? SettingsPalette.selectedStroke : SettingsPalette.subtleBorder, lineWidth: 1)
            )
        }
        .buttonStyle(.plain)
    }

    private var statusColor: Color {
        switch status {
        case .offline:
            return .secondary
        case .online:
            return .green
        case .connected:
            return .blue
        case .error:
            return .red
        }
    }
}

struct SettingsFieldLabel: View {
    let text: String

    init(_ text: String) {
        self.text = text
    }

    var body: some View {
        Text(text)
            .font(.body.weight(.medium))
            .frame(width: 150, alignment: .leading)
    }
}

struct StatusPill: View {
    let label: String
    let status: ConnectionStatus?

    var body: some View {
        Text(label)
            .font(.caption.weight(.semibold))
            .padding(.horizontal, 8)
            .padding(.vertical, 5)
            .background(
                Capsule()
                    .fill(fillColor)
            )
            .foregroundStyle(foregroundColor)
    }

    private var fillColor: Color {
        switch status {
        case .offline:
            return Color.secondary.opacity(0.15)
        case .online:
            return Color.green.opacity(0.15)
        case .connected:
            return Color.blue.opacity(0.15)
        case .error:
            return Color.red.opacity(0.15)
        case nil:
            return Color.secondary.opacity(0.12)
        }
    }

    private var foregroundColor: Color {
        switch status {
        case .error:
            return .red
        case .online:
            return .green
        case .connected:
            return .blue
        default:
            return .secondary
        }
    }
}

struct LogEmptyState: View {
    let text: String

    init(_ text: String) {
        self.text = text
    }

    var body: some View {
        Text(text)
            .font(.callout)
            .foregroundStyle(.secondary)
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
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
        window.setContentSize(NSSize(width: 1_080, height: 760))
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
