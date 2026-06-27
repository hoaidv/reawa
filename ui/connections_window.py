from __future__ import annotations

import threading

import objc
from AppKit import (
    NSApplication,
    NSBackingStoreBuffered,
    NSButton,
    NSControlTextDidChangeNotification,
    NSMakeRect,
    NSMenu,
    NSMenuItem,
    NSNotificationCenter,
    NSScrollView,
    NSSegmentedControl,
    NSTableColumn,
    NSTableView,
    NSTabView,
    NSTabViewItem,
    NSTextField,
    NSView,
    NSWindow,
    NSWindowStyleMaskClosable,
    NSWindowStyleMaskTitled,
)
from Foundation import NSIndexSet
from PyObjCTools import AppHelper

from remarkable.models.connection import Connection, ConnectionStatus
from remarkable.services.connection_manager import ConnectionManager
from remarkable.services.network_discovery import discover_usb_ssh_hosts
from remarkable.ui.log_panel import LogPanelController


class ConnectionsWindowController(objc.lookUpClass("NSObject")):
    def initWithManager_(self, manager):
        self = objc.super(ConnectionsWindowController, self).init()
        if self is None:
            return None
        self.manager = manager
        self._window = None
        self._table = None
        self._connections: list[Connection] = []
        self._selected_id = None
        self.on_change = None
        self.on_open = None
        self.on_close = None
        self.on_mode_changed = None
        self._form_snapshot: dict | None = None
        self._loading = False

        self.name_field = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 200, 24))
        self.ip_field = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 200, 24))
        self.password_field = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 200, 24))
        self.password_field.setPlaceholderString_("Password (add only)")
        self.auto_connect_btn = NSButton.alloc().initWithFrame_(NSMakeRect(0, 0, 200, 24))
        self.auto_connect_btn.setButtonType_(3)
        self.auto_connect_btn.setTitle_("Auto-connect on USB detect")
        self.mode_control = NSSegmentedControl.alloc().initWithFrame_(NSMakeRect(0, 0, 320, 24))
        self.mode_control.setSegmentCount_(2)
        self.mode_control.setLabel_forSegment_("Relative", 0)
        self.mode_control.setLabel_forSegment_("Absolute", 1)
        self.mode_control.setSelectedSegment_(0)
        self.mode_control.setTarget_(self)
        self.mode_control.setAction_("modeChanged:")
        self.scale_field = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 200, 24))
        self.scale_field.setPlaceholderString_("Scale (empty = auto)")
        self.swap_btn = NSButton.alloc().initWithFrame_(NSMakeRect(0, 0, 120, 24))
        self.swap_btn.setButtonType_(3)
        self.swap_btn.setTitle_("Swap XY")
        self.invert_x_btn = NSButton.alloc().initWithFrame_(NSMakeRect(0, 0, 120, 24))
        self.invert_x_btn.setButtonType_(3)
        self.invert_x_btn.setTitle_("Invert X")
        self.invert_y_btn = NSButton.alloc().initWithFrame_(NSMakeRect(0, 0, 120, 24))
        self.invert_y_btn.setButtonType_(3)
        self.invert_y_btn.setTitle_("Invert Y")
        self.border_color_field = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 200, 24))
        self.border_color_field.setPlaceholderString_("#3B82F6")

        self.snapped_window_label = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 320, 22))
        self.snapped_window_label.setEditable_(False)
        self.snapped_window_label.setBezeled_(False)
        self.snapped_window_label.setDrawsBackground_(False)
        self.snapped_window_label.setStringValue_("Snapped window: —")

        self._discovered_table = None
        self._discovered_scroll = None
        self._discovered_header = None

        self.form_header = NSTextField.alloc().initWithFrame_(NSMakeRect(0, 0, 320, 22))
        self.form_header.setEditable_(False)
        self.form_header.setBezeled_(False)
        self.form_header.setDrawsBackground_(False)
        self.form_header.setStringValue_("New connection")

        self._discovered_ips: list[str] = []
        self._scanning = False
        # None => the form is composing a NEW connection; otherwise editing this id.
        self._editing_id: str | None = None
        self._log_panel = LogPanelController.alloc().init()
        self._tab_view = None
        return self

    def show(self) -> None:
        if self._window is None:
            self._build_window()
        self._reload_table()
        self._window.makeKeyAndOrderFront_(None)
        self._window.center()
        from AppKit import NSApp

        NSApp().activateIgnoringOtherApps_(True)
        if self.on_open:
            self.on_open()
        self._start_scan()

    def windowWillClose_(self, notification) -> None:
        if self.on_close:
            self.on_close()

    def _build_window(self) -> None:
        # The connections layout uses absolute coordinates up to ~512pt tall, so
        # the tab view's *content* area must be at least that high. NSTabView
        # reserves space at the top for the tab strip, so the window content view
        # is made taller than the panel to compensate.
        panel_w = 720
        panel_h = 520
        window_frame = NSMakeRect(200, 200, panel_w, panel_h + 48)
        self._window = NSWindow.alloc().initWithContentRect_styleMask_backing_defer_(
            window_frame,
            NSWindowStyleMaskTitled | NSWindowStyleMaskClosable,
            NSBackingStoreBuffered,
            False,
        )
        self._window.setTitle_("Reawa Settings")
        # Closing the window must not deallocate it; we reuse the instance.
        # Otherwise reopening dereferences freed memory and segfaults.
        self._window.setReleasedWhenClosed_(False)
        self._window.setDelegate_(self)
        _install_edit_menu()

        content_view = self._window.contentView()
        self._tab_view = NSTabView.alloc().initWithFrame_(content_view.bounds())
        self._tab_view.setAutoresizingMask_(18)  # width + height resizable
        self._tab_view.setDelegate_(self)

        # Size each panel to the tab view's content rect so nothing is clipped
        # behind the tab strip.
        panel_frame = self._tab_view.contentRect()

        connections_tab = NSTabViewItem.alloc().initWithIdentifier_("connections")
        connections_tab.setLabel_("Connections")
        connections_tab.setView_(self._build_connections_panel(panel_frame))
        self._tab_view.addTabViewItem_(connections_tab)

        logs_tab = NSTabViewItem.alloc().initWithIdentifier_("logs")
        logs_tab.setLabel_("Logs")
        logs_tab.setView_(self._log_panel.build_view(panel_frame))
        self._tab_view.addTabViewItem_(logs_tab)

        content_view.addSubview_(self._tab_view)

    def tabView_didSelectTabViewItem_(self, tabView, item):
        if item.label() == "Logs":
            self._log_panel.note_visible()

    def _build_connections_panel(self, frame) -> NSView:
        content = NSView.alloc().initWithFrame_(frame)

        left_x = 20
        left_w = 300

        # Connections list (lower section).
        conn_header = NSTextField.alloc().initWithFrame_(NSMakeRect(left_x, 352, left_w, 18))
        conn_header.setEditable_(False)
        conn_header.setBezeled_(False)
        conn_header.setDrawsBackground_(False)
        conn_header.setStringValue_("Connections")
        content.addSubview_(conn_header)

        self._table = NSTableView.alloc().initWithFrame_(NSMakeRect(0, 0, left_w, 120))
        conn_scroll = NSScrollView.alloc().initWithFrame_(NSMakeRect(left_x, 220, left_w, 125))
        conn_scroll.setDocumentView_(self._table)
        conn_scroll.setHasVerticalScroller_(True)
        conn_scroll.setBorderType_(1)
        content.addSubview_(conn_scroll)

        buttons = [
            ("New", "newConnection:", left_x),
            ("Remove", "removeConnection:", left_x + 90),
            ("Connect", "connectConnection:", left_x + 180),
        ]
        for title, action, x in buttons:
            btn = NSButton.alloc().initWithFrame_(NSMakeRect(x, 180, 86, 28))
            btn.setTitle_(title)
            btn.setTarget_(self)
            btn.setAction_(action)
            content.addSubview_(btn)
        self._connect_button = content.subviews()[-1]

        # Discovered devices (upper section, aligned with connections list).
        self._discovered_header = NSTextField.alloc().initWithFrame_(
            NSMakeRect(left_x, 458, left_w, 18)
        )
        self._discovered_header.setEditable_(False)
        self._discovered_header.setBezeled_(False)
        self._discovered_header.setDrawsBackground_(False)
        self._discovered_header.setStringValue_("Discovered")
        content.addSubview_(self._discovered_header)

        self._discovered_table = NSTableView.alloc().initWithFrame_(NSMakeRect(0, 0, left_w, 80))
        self._discovered_scroll = NSScrollView.alloc().initWithFrame_(
            NSMakeRect(left_x, 378, left_w, 75)
        )
        self._discovered_scroll.setDocumentView_(self._discovered_table)
        self._discovered_scroll.setHasVerticalScroller_(True)
        self._discovered_scroll.setBorderType_(1)
        content.addSubview_(self._discovered_scroll)

        scan_btn = NSButton.alloc().initWithFrame_(NSMakeRect(left_x, 484, 110, 28))
        scan_btn.setTitle_("Scan devices")
        scan_btn.setTarget_(self)
        scan_btn.setAction_("scanDevices:")
        content.addSubview_(scan_btn)

        self.form_header.setFrame_(NSMakeRect(360, 462, 320, 22))
        content.addSubview_(self.form_header)

        layouts = [
            (self.name_field, 430),
            (self.ip_field, 390),
            (self.password_field, 350),
            (self.auto_connect_btn, 310),
            (self.mode_control, 270),
            (self.scale_field, 230),
            (self.swap_btn, 190),
            (self.invert_x_btn, 190),
            (self.invert_y_btn, 190),
            (self.border_color_field, 150),
            (self.snapped_window_label, 118),
        ]
        x_positions = {
            self.swap_btn: 360,
            self.invert_x_btn: 470,
            self.invert_y_btn: 580,
        }
        for widget, y in layouts:
            x = x_positions.get(widget, 360)
            w = 100 if widget in (self.swap_btn, self.invert_x_btn, self.invert_y_btn) else 320
            widget.setFrame_(NSMakeRect(x, y, w, 24))
            content.addSubview_(widget)

        self._wire_form_change_notifications()

        # Single Save button commits the form (creates when new, updates when editing).
        self._save_button = NSButton.alloc().initWithFrame_(NSMakeRect(360, 70, 320, 32))
        self._save_button.setTitle_("Save")
        self._save_button.setTarget_(self)
        self._save_button.setAction_("saveConnection:")
        self._save_button.setKeyEquivalent_("\r")  # Enter triggers Save
        content.addSubview_(self._save_button)

        self._table.setDelegate_(self)
        self._table.setDataSource_(self)
        self._discovered_table.setDelegate_(self)
        self._discovered_table.setDataSource_(self)
        if not self._discovered_table.tableColumns():
            col = NSTableColumn.alloc().initWithIdentifier_("ip")
            col.setWidth_(left_w - 24)
            self._discovered_table.addTableColumn_(col)
        self._update_discovered_header()
        self._update_form_header()
        self._update_absolute_panel()
        self._update_save_button()
        if getattr(self, "_connect_button", None) is not None:
            self._connect_button.setEnabled_(False)
        return content

    def _wire_form_change_notifications(self) -> None:
        for field in (
            self.name_field,
            self.ip_field,
            self.password_field,
            self.scale_field,
            self.border_color_field,
        ):
            NSNotificationCenter.defaultCenter().addObserver_selector_name_object_(
                self,
                "formFieldChanged:",
                NSControlTextDidChangeNotification,
                field,
            )
        for btn in (self.auto_connect_btn, self.swap_btn, self.invert_x_btn, self.invert_y_btn):
            btn.setTarget_(self)
            btn.setAction_("formFieldChanged:")

    @objc.IBAction
    def formFieldChanged_(self, sender):
        self._update_save_button()

    @objc.IBAction
    def modeChanged_(self, sender):
        if self._loading or self._editing_id is None:
            return
        conn = self.manager.get_connection(self._editing_id)
        if conn is None:
            return
        new_mode = "ABSOLUTE" if self.mode_control.selectedSegment() == 1 else "RELATIVE"
        if new_mode == conn.device_config.output_mode:
            return
        conn.device_config.output_mode = new_mode
        self.manager.update_connection(conn)
        if self.on_mode_changed:
            self.on_mode_changed(self._editing_id, new_mode)
        self._update_absolute_panel()
        self._update_save_button()

    def refresh_absolute_panel(self, picking: bool = False) -> None:
        self._update_absolute_panel(picking=picking)

    def _update_absolute_panel(self, picking: bool = False) -> None:
        if not getattr(self, "snapped_window_label", None):
            return
        is_absolute = self.mode_control.selectedSegment() == 1
        self.snapped_window_label.setHidden_(not is_absolute)

        if not is_absolute:
            return

        conn = None
        if self._editing_id:
            conn = self.manager.get_connection(self._editing_id)
        elif self._selected_id:
            conn = self.manager.get_connection(self._selected_id)

        if picking:
            self.snapped_window_label.setStringValue_("Snapped window: (selecting…)")
            return

        ref = conn.device_config.absolute.snapped_window_ref if conn else None
        if ref:
            self.snapped_window_label.setStringValue_(f"Snapped window: {ref}")
        else:
            self.snapped_window_label.setStringValue_("Snapped window: (none — pick a window)")

    def _current_form_state(self) -> dict:
        scale_text = self.scale_field.stringValue().strip()
        scale: float | str | None = None
        if scale_text:
            try:
                scale = float(scale_text)
            except ValueError:
                scale = scale_text
        return {
            "name": self.name_field.stringValue().strip(),
            "ip": self.ip_field.stringValue().strip(),
            "auto_connect": bool(self.auto_connect_btn.state()),
            "scale": scale,
            "swap_xy": bool(self.swap_btn.state()),
            "invert_x": bool(self.invert_x_btn.state()),
            "invert_y": bool(self.invert_y_btn.state()),
            "border_color": self.border_color_field.stringValue().strip() or "#3B82F6",
        }

    def _capture_form_snapshot(self) -> None:
        self._form_snapshot = self._current_form_state()

    def _is_form_dirty(self) -> bool:
        if self._editing_id is None:
            return True
        if self._form_snapshot is None:
            return False
        return self._current_form_state() != self._form_snapshot

    def _update_save_button(self) -> None:
        if not getattr(self, "_save_button", None):
            return
        if self._editing_id is None:
            ready = bool(
                self.name_field.stringValue().strip()
                and self.ip_field.stringValue().strip()
                and self.password_field.stringValue()
            )
            self._save_button.setEnabled_(ready)
            return
        self._save_button.setEnabled_(self._is_form_dirty())

    @objc.IBAction
    def scanDevices_(self, sender):
        self._start_scan()

    def _saved_connection_ips(self) -> set[str]:
        return {c.ip for c in self.manager.list_connections() if c.ip}

    def _update_discovered_header(self) -> None:
        if not self._discovered_header:
            return
        if self._scanning:
            suffix = " — scanning…"
        elif not self._discovered_ips:
            suffix = " — none"
        else:
            suffix = ""
        self._discovered_header.setStringValue_(f"Discovered{suffix}")

    def _reload_discovered_table(self) -> None:
        self._update_discovered_header()
        if self._discovered_table:
            self._discovered_table.reloadData()

    def _refresh_discovered_after_save(self, saved_ip: str) -> None:
        self._discovered_ips = [ip for ip in self._discovered_ips if ip != saved_ip]
        self._reload_discovered_table()

    def _start_scan(self) -> None:
        if self._scanning:
            return
        self._scanning = True
        self._reload_discovered_table()
        threading.Thread(target=self._scan_worker, daemon=True).start()

    def _scan_worker(self) -> None:
        try:
            ips = sorted(discover_usb_ssh_hosts())
        except Exception as exc:
            ips = []
            print(f"Scan failed: {exc}")
        saved_ips = self._saved_connection_ips()
        filtered = [ip for ip in ips if ip not in saved_ips]
        AppHelper.callAfter(self._scan_done, filtered)

    def _scan_done(self, ips) -> None:
        self._scanning = False
        self._discovered_ips = list(ips)
        self._reload_discovered_table()

    def _clear_form(self) -> None:
        """Reset the form to compose a new connection with no selection."""
        self._loading = True
        try:
            self._editing_id = None
            self._selected_id = None
            if self._table:
                self._table.deselectAll_(None)
            if self._discovered_table:
                self._discovered_table.deselectAll_(None)
            self.name_field.setStringValue_("")
            self.ip_field.setStringValue_("")
            self.password_field.setStringValue_("")
            self.auto_connect_btn.setState_(0)
            self.mode_control.setSelectedSegment_(0)
            self.scale_field.setStringValue_("")
            self.swap_btn.setState_(0)
            self.invert_x_btn.setState_(0)
            self.invert_y_btn.setState_(0)
            self.border_color_field.setStringValue_("#3B82F6")
            self._form_snapshot = None
            if getattr(self, "_connect_button", None) is not None:
                self._connect_button.setEnabled_(False)
                self._connect_button.setTitle_("Connect")
            self._update_form_header()
            self._update_absolute_panel()
        finally:
            self._loading = False
        self._update_save_button()

    def _load_discovered_ip(self, ip: str) -> None:
        """Fill the form to add a new connection for a discovered device."""
        self._loading = True
        try:
            self._editing_id = None
            self._selected_id = None
            if self._table:
                self._table.deselectAll_(None)
            self.name_field.setStringValue_("reMarkable")
            self.ip_field.setStringValue_(ip)
            self.password_field.setStringValue_("")
            self.auto_connect_btn.setState_(0)
            self.mode_control.setSelectedSegment_(0)
            self.scale_field.setStringValue_("")
            self.swap_btn.setState_(0)
            self.invert_x_btn.setState_(0)
            self.invert_y_btn.setState_(0)
            self.border_color_field.setStringValue_("#3B82F6")
            self._form_snapshot = None
            if getattr(self, "_connect_button", None) is not None:
                self._connect_button.setEnabled_(False)
                self._connect_button.setTitle_("Connect")
            self._update_form_header()
            self._update_absolute_panel()
        finally:
            self._loading = False
        self._update_save_button()
        self._window.makeFirstResponder_(self.password_field)

    @objc.IBAction
    def newConnection_(self, sender):
        """Clear the form to compose a brand-new connection."""
        self._clear_form()
        self._window.makeFirstResponder_(self.name_field)

    @objc.IBAction
    def connectConnection_(self, sender):
        if not self._selected_id:
            return
        try:
            self.manager.toggle_connection(self._selected_id)
        except Exception as exc:
            print(f"Connect failed: {exc}")
        self._reload_table()
        if self.on_change:
            self.on_change()

    @objc.IBAction
    def removeConnection_(self, sender):
        if not self._selected_id:
            return
        self.manager.remove_connection(self._selected_id)
        self._selected_id = None
        self._editing_id = None
        self._reload_table()
        self._start_scan()
        if self.on_change:
            self.on_change()

    @objc.IBAction
    def saveConnection_(self, sender):
        """Create a new connection (when composing) or update the selected one."""
        if self._editing_id is None:
            self._save_new()
        else:
            self._save_existing(self._editing_id)

    def _save_new(self) -> None:
        name = self.name_field.stringValue().strip() or "reMarkable"
        ip = self.ip_field.stringValue().strip() or "10.11.99.1"
        password = self.password_field.stringValue()
        if not password:
            self.form_header.setStringValue_("New connection — password required")
            self._window.makeFirstResponder_(self.password_field)
            return
        try:
            conn = self.manager.add_connection(
                name=name,
                ip=ip,
                password=password,
                auto_connect=bool(self.auto_connect_btn.state()),
            )
            # Persist the device-config fields entered on the same form.
            self._apply_device_config(conn)
            self.manager.update_connection(conn)
            self._editing_id = conn.id
            self._selected_id = conn.id
            if self._discovered_table:
                self._discovered_table.deselectAll_(None)
            self._capture_form_snapshot()
            self._update_form_header()
            self._update_save_button()
            self._reload_table()
            self._refresh_discovered_after_save(conn.ip)
            if getattr(self, "_connect_button", None) is not None:
                self._connect_button.setEnabled_(True)
                self._connect_button.setTitle_("Connect")
            if self.on_change:
                self.on_change()
        except Exception as exc:
            self.form_header.setStringValue_(f"Add failed: {exc}")
            print(f"Add failed: {exc}")

    def _save_existing(self, connection_id: str) -> None:
        conn = self.manager.get_connection(connection_id)
        if not conn:
            return
        conn.name = self.name_field.stringValue().strip() or conn.name
        conn.ip = self.ip_field.stringValue().strip() or conn.ip
        conn.auto_connect = bool(self.auto_connect_btn.state())
        self._apply_device_config(conn)
        self.manager.update_connection(conn)
        self._capture_form_snapshot()
        self._update_form_header()
        self._update_save_button()
        self._reload_table()
        if self.on_change:
            self.on_change()

    def _apply_device_config(self, conn: Connection) -> None:
        if self._editing_id is None:
            conn.device_config.output_mode = (
                "ABSOLUTE" if self.mode_control.selectedSegment() == 1 else "RELATIVE"
            )
        scale_text = self.scale_field.stringValue().strip()
        try:
            conn.device_config.scale = float(scale_text) if scale_text else None
        except ValueError:
            conn.device_config.scale = None
        conn.device_config.swap_xy = bool(self.swap_btn.state())
        conn.device_config.invert_x = bool(self.invert_x_btn.state())
        conn.device_config.invert_y = bool(self.invert_y_btn.state())
        conn.device_config.absolute.border_color = (
            self.border_color_field.stringValue().strip() or "#3B82F6"
        )

    def _update_form_header(self) -> None:
        if self._editing_id is None:
            self.form_header.setStringValue_("New connection (enter password, then Save)")
            if getattr(self, "_save_button", None) is not None:
                self._save_button.setTitle_("Add connection")
        else:
            conn = self.manager.get_connection(self._editing_id)
            name = conn.name if conn else "connection"
            self.form_header.setStringValue_(f"Editing: {name}")
            if getattr(self, "_save_button", None) is not None:
                self._save_button.setTitle_("Save changes")

    def refresh_status(self) -> None:
        """Refresh connection list status labels without disturbing the form."""
        if self._window is None or not self._window.isVisible():
            return
        self._connections = self.manager.list_connections()
        if self._table:
            self._table.reloadData()
        if self._selected_id and getattr(self, "_connect_button", None) is not None:
            is_active = self.manager.status(self._selected_id) == ConnectionStatus.CONNECTED
            self._connect_button.setTitle_("Disconnect" if is_active else "Connect")

    def _reload_table(self) -> None:
        prev_id = self._selected_id or self._editing_id
        self._connections = self.manager.list_connections()
        if self._table:
            if not self._table.tableColumns():
                col = NSTableColumn.alloc().initWithIdentifier_("name")
                col.setWidth_(280)
                self._table.addTableColumn_(col)
            self._table.reloadData()
        if prev_id:
            for i, conn in enumerate(self._connections):
                if conn.id == prev_id:
                    self._table.selectRowIndexes_byExtendingSelection_(
                        NSIndexSet.indexSetWithIndex_(i), False
                    )
                    return
        if self._connections:
            self._load_connection(self._connections[0])
        else:
            self._clear_form()

    def _load_connection(self, conn: Connection) -> None:
        self._loading = True
        try:
            self._selected_id = conn.id
            self._editing_id = conn.id
            self._update_form_header()
            self.name_field.setStringValue_(conn.name)
            self.ip_field.setStringValue_(conn.ip)
            self.password_field.setStringValue_("")
            self.auto_connect_btn.setState_(1 if conn.auto_connect else 0)
            self.mode_control.setSelectedSegment_(
                1 if conn.device_config.output_mode == "ABSOLUTE" else 0
            )
            self.scale_field.setStringValue_(
                "" if conn.device_config.scale is None else str(conn.device_config.scale)
            )
            self.swap_btn.setState_(1 if conn.device_config.swap_xy else 0)
            self.invert_x_btn.setState_(1 if conn.device_config.invert_x else 0)
            self.invert_y_btn.setState_(1 if conn.device_config.invert_y else 0)
            self.border_color_field.setStringValue_(conn.device_config.absolute.border_color)

            if getattr(self, "_connect_button", None) is not None:
                is_active = self.manager.status(conn.id) == ConnectionStatus.CONNECTED
                self._connect_button.setTitle_("Disconnect" if is_active else "Connect")
                self._connect_button.setEnabled_(True)
            self._capture_form_snapshot()
            self._update_absolute_panel()
        finally:
            self._loading = False
        self._update_save_button()

    def numberOfRowsInTableView_(self, tableView):
        if tableView is self._discovered_table:
            return len(self._discovered_ips)
        return len(self._connections)

    def tableView_objectValueForTableColumn_row_(self, tableView, column, row):
        if tableView is self._discovered_table:
            return self._discovered_ips[row]
        conn = self._connections[row]
        status = self.manager.status(conn.id).value
        return f"{conn.name} ({conn.ip}) — {status}"

    def tableViewSelectionDidChange_(self, notification):
        table = notification.object()
        if table is self._discovered_table:
            row = self._discovered_table.selectedRow()
            if 0 <= row < len(self._discovered_ips):
                self._load_discovered_ip(self._discovered_ips[row])
            return
        row = self._table.selectedRow()
        if 0 <= row < len(self._connections):
            if self._discovered_table:
                self._discovered_table.deselectAll_(None)
            self._load_connection(self._connections[row])

    def note_mode_changed(self, conn_id: str) -> None:
        """Reflect an externally-driven mode change (e.g. snap cancelled)."""
        if self._window is None or self._editing_id != conn_id:
            return
        conn = self.manager.get_connection(conn_id)
        if conn is not None:
            self._loading = True
            try:
                self.mode_control.setSelectedSegment_(
                    1 if conn.device_config.output_mode == "ABSOLUTE" else 0
                )
                self._update_absolute_panel()
            finally:
                self._loading = False
            self._update_save_button()


def _install_edit_menu() -> None:
    """Install a standard Edit menu so Cut/Copy/Paste/Select All work.

    rumps only builds the status-bar menu, never the app's main menu. Without
    an Edit menu, ⌘V has no ``paste:`` menu item to dispatch, so pasting into
    text fields silently fails. Items use a nil target, so the actions travel
    up the responder chain to the focused field's editor.
    """
    app = NSApplication.sharedApplication()
    main_menu = app.mainMenu()
    if main_menu is None:
        main_menu = NSMenu.alloc().init()
        app.setMainMenu_(main_menu)

    for item in main_menu.itemArray():
        submenu = item.submenu()
        if submenu is not None and submenu.title() == "Edit":
            return

    edit_item = NSMenuItem.alloc().init()
    edit_menu = NSMenu.alloc().initWithTitle_("Edit")

    entries = [
        ("Cut", "cut:", "x"),
        ("Copy", "copy:", "c"),
        ("Paste", "paste:", "v"),
        ("Select All", "selectAll:", "a"),
    ]
    for title, action, key in entries:
        mi = NSMenuItem.alloc().initWithTitle_action_keyEquivalent_(title, action, key)
        mi.setTarget_(None)  # dispatch via responder chain to the focused field
        edit_menu.addItem_(mi)

    edit_item.setSubmenu_(edit_menu)
    main_menu.addItem_(edit_item)


def create_connections_window(manager: ConnectionManager):
    return ConnectionsWindowController.alloc().initWithManager_(manager)
