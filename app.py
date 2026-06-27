from __future__ import annotations

import os

import rumps
from PyObjCTools import AppHelper

from remarkable.driver.window_snap import WindowSnapController
from remarkable.models.connection import ConnectionStatus
from remarkable.services.connection_manager import ConnectionManager
from remarkable.services.notifications import NotificationService
from remarkable.services.usb_watcher import USBWatcher
from remarkable.ui.connections_window import create_connections_window
from remarkable.ui.dock_policy import set_dock_visible
from remarkable.ui.region_overlay import RegionOverlayController
from remarkable.ui.snap_picker import SnapPicker


STATUS_PREFIX = {
    ConnectionStatus.OFFLINE: "○",
    ConnectionStatus.ONLINE: "◎",
    ConnectionStatus.CONNECTED: "●",
    ConnectionStatus.ERROR: "✗",
}

def _asset_path(name: str) -> str:
    # In a py2app bundle the package is zipped, so __file__-relative assets
    # don't exist on disk. py2app sets RESOURCEPATH to Contents/Resources,
    # where bundled data files live (see packaging/setup.py DATA_FILES).
    resource_root = os.environ.get("RESOURCEPATH")
    if resource_root:
        bundled = os.path.join(resource_root, "assets", name)
        if os.path.exists(bundled):
            return bundled
    return os.path.join(os.path.dirname(__file__), "assets", name)


MENU_ICON = _asset_path("menu_icon.png")

ABOUT_MESSAGE = """Reawa 0.2.0

Reawa turns a reMarkable 2 into a macOS pen input device.

License: MIT

reMarkable is a registered trademark of reMarkable AS. Wacom is a registered
trademark of Wacom Co., Ltd. Reawa is an independent project and is not
affiliated with, endorsed by, or sponsored by reMarkable AS, Wacom Co., Ltd.,
or Apple Inc.

See the bundled LICENSE, NOTICE, and THIRD_PARTY_LICENSES files for legal and
third-party software notices."""


class RemarkableApp(rumps.App):
    def __init__(self) -> None:
        icon = MENU_ICON if os.path.exists(MENU_ICON) else None
        super().__init__("Reawa", icon=icon, template=True, quit_button=None)
        # Icon replaces the text title; a dot is appended only while connected.
        self.title = None
        set_dock_visible(False)
        self.manager = ConnectionManager()
        self.notifications = NotificationService()
        self.window_snap = WindowSnapController()
        self.picker = SnapPicker()

        self.connections_window = create_connections_window(self.manager)
        self.connections_window.on_change = self.refresh_menu
        self.connections_window.on_open = lambda: set_dock_visible(True)
        self.connections_window.on_close = lambda: set_dock_visible(False)
        self.connections_window.on_mode_changed = self._on_settings_mode_changed

        self.region_overlay = RegionOverlayController(self._on_region_changed)
        self.usb_watcher = USBWatcher(self.manager, self.notifications)
        self.usb_watcher.set_on_detected(lambda _c: self.refresh_menu())

        self.manager.add_listener(self.refresh_menu)

        self._picking = False
        self._snapped_conn_id: str | None = None
        self._snapped_window_state: str = "normal"  # normal | minimized
        self._follow_timer = rumps.Timer(self._follow_tick, 0.4)

        self.usb_watcher.start()
        self.refresh_menu()

    def _active_connection(self):
        conn_id = self.manager.active_connection_id()
        if not conn_id:
            return None
        return self.manager.get_connection(conn_id)

    def _on_region_changed(self, region) -> None:
        conn = self._active_connection()
        if not conn:
            return
        conn.device_config.absolute = region
        self.manager.update_connection(conn)
        self.window_snap.sync_window_to_region(region)

    def _start_pick(self, conn_id: str) -> None:
        if self._picking:
            return
        self._picking = True
        self.manager.pause_input()
        self.region_overlay.hide()
        self._stop_follow()
        self.connections_window.refresh_absolute_panel(picking=True)
        self.picker.start(on_pick=self._on_picked, on_cancel=self._on_pick_cancel)

    def _stop_follow(self) -> None:
        if self._follow_timer.is_alive():
            self._follow_timer.stop()

    def _cancel_pick(self) -> None:
        """Tear down an in-progress picker and clear the picking flag.

        Without this, leaving the picker via anything other than Esc/click
        (e.g. the Relative/Release control) leaves ``_picking`` stuck True,
        which permanently blocks the snapping UX from starting again.
        """
        if self._picking:
            self.picker.stop()
            self._picking = False

    def _on_picked(self, info) -> None:
        conn = self._active_connection()
        if conn is None:
            self._picking = False
            return

        ref, wx, wy, ww, wh = self.window_snap.pick_from_info(info)
        # If the picked window is minimized (incl. a tiny Stage Manager
        # thumbnail), restore it and snap to its real, settled frame.
        restored = self.window_snap.restore_window()
        if restored is not None:
            wx, wy, ww, wh = restored
        region = conn.device_config.absolute
        self.window_snap.snap_region_to_window(region, wx, wy, ww, wh)
        self.window_snap.sync_window_to_region(region)
        region.snap_window_enabled = True
        region.snapped_window_ref = ref
        conn.device_config.output_mode = "ABSOLUTE"
        self.manager.update_connection(conn)

        self._picking = False
        self._snapped_conn_id = conn.id
        self._snapped_window_state = "normal"
        # The live session swaps to the AbsoluteDriver on its own once input
        # resumes and it sees the ABSOLUTE config — no reconnect needed.
        self.manager.resume_input()
        self.connections_window.refresh_absolute_panel()
        self.refresh_menu()

    def _on_pick_cancel(self) -> None:
        self._picking = False
        conn = self._active_connection()
        if conn is None:
            return
        self._revert_to_relative(conn.id)

    def _switch_to_relative(self) -> None:
        conn = self._active_connection()
        if conn is not None:
            self._revert_to_relative(conn.id)

    def _switch_to_absolute(self) -> None:
        """Enter ABSOLUTE for the active connection and start the snap picker."""
        conn = self._active_connection()
        if conn is None:
            return
        conn.device_config.output_mode = "ABSOLUTE"
        self.manager.update_connection(conn)
        self.connections_window.note_mode_changed(conn.id)
        # Force the snapping UX even if a window was previously snapped.
        self._snapped_conn_id = None
        self._cancel_pick()
        # _update_overlay (via refresh_menu) starts the picker for ABSOLUTE.
        self.refresh_menu()

    def _on_settings_mode_changed(self, conn_id: str, mode: str) -> None:
        if mode == "RELATIVE":
            if self.manager.active_connection_id() == conn_id:
                self._revert_to_relative(conn_id)
            else:
                conn = self.manager.get_connection(conn_id)
                if conn is not None:
                    conn.device_config.absolute.snap_window_enabled = False
                    conn.device_config.absolute.snapped_window_ref = None
                    self.manager.update_connection(conn)
                self.refresh_menu()
            return

        # ABSOLUTE — enter snapping UX for the active connection immediately.
        if self.manager.active_connection_id() == conn_id:
            self._snapped_conn_id = None
            self._cancel_pick()
        self.refresh_menu()

    def _restart_pick(self) -> None:
        conn = self._active_connection()
        if conn is None:
            return
        self._cancel_pick()
        self._snapped_conn_id = None
        self._start_pick(conn.id)

    def _revert_to_relative(self, conn_id: str) -> None:
        self._cancel_pick()
        conn = self.manager.get_connection(conn_id)
        if conn is None:
            return
        conn.device_config.output_mode = "RELATIVE"
        conn.device_config.absolute.snap_window_enabled = False
        conn.device_config.absolute.snapped_window_ref = None
        self.window_snap.clear()
        self._snapped_conn_id = None
        self._snapped_window_state = "normal"
        self.manager.update_connection(conn)
        self.region_overlay.hide()
        self._stop_follow()
        # Make sure input isn't left paused if we cancelled out of the picker.
        self.manager.resume_input()
        self.connections_window.note_mode_changed(conn_id)
        self.refresh_menu()

    def _follow_tick(self, _timer) -> None:
        if self._snapped_conn_id is None or self._picking:
            return
        conn = self._active_connection()
        if conn is None or conn.device_config.output_mode != "ABSOLUTE":
            return

        lifecycle = self.window_snap.snapped_lifecycle_state()
        region = conn.device_config.absolute

        if lifecycle == "closed":
            self._revert_to_relative(conn.id)
            return

        if lifecycle == "minimized":
            if self._snapped_window_state != "minimized":
                self.region_overlay.hide()
                self._snapped_window_state = "minimized"
            return

        if self._snapped_window_state == "minimized":
            self._snapped_window_state = "normal"
            if self.window_snap.sync_region_to_window(region):
                self.manager.update_connection(conn)
            self.region_overlay.show(region)

        if lifecycle == "maximized":
            frame = self.window_snap.current_window_frame()
            if frame is not None:
                wx, wy, ww, wh = frame
                self.window_snap.snap_region_to_window(region, wx, wy, ww, wh)
                self.window_snap.sync_window_to_region(region)
                self.manager.update_connection(conn)
                self.region_overlay.show(region)
            return

        frame = self.window_snap.current_window_frame()
        if frame is None:
            return
        fx, fy, fw, fh = frame
        moved = abs(fx - region.region_x) > 2 or abs(fy - region.region_y) > 2
        resized = abs(fw - region.region_width) > 2 or abs(fh - region.region_height) > 2
        if moved or resized:
            if resized:
                self.window_snap.snap_region_to_window(region, fx, fy, fw, fh)
                self.window_snap.sync_window_to_region(region)
            else:
                region.region_x = fx
                region.region_y = fy
            self.region_overlay.update_region(region)
            self.manager.update_connection(conn)

    def refresh_menu(self) -> None:
        AppHelper.callAfter(self._refresh_menu_main)

    def _refresh_menu_main(self) -> None:
        new_menu: list = []
        for conn in self.manager.list_connections():
            status = self.manager.status(conn.id)
            prefix = STATUS_PREFIX.get(status, "○")
            item = rumps.MenuItem(
                f"{prefix} {conn.name}",
                callback=self._make_connect_handler(conn.id),
            )
            new_menu.append(item)

        new_menu.append(None)
        new_menu.extend(self._build_mode_items())
        new_menu.append(None)
        new_menu.append(rumps.MenuItem("Open", callback=self.open_window))
        new_menu.append(rumps.MenuItem("About Reawa", callback=self.show_about))
        new_menu.append(rumps.MenuItem("Quit", callback=self.quit_app))

        self.menu.clear()
        self.menu = new_menu

        self._update_overlay()
        self.connections_window.refresh_status()

    def _active_mode(self) -> str | None:
        """Output mode of the active (connected) connection, else None."""
        conn = self._active_connection()
        if conn is None:
            return None
        if self.manager.status(conn.id) != ConnectionStatus.CONNECTED:
            return None
        return conn.device_config.output_mode

    def _snapped_window_ref(self) -> str | None:
        conn = self._active_connection()
        if conn is None:
            return None
        return conn.device_config.absolute.snapped_window_ref

    @staticmethod
    def _sending_to_label(window_ref: str | None) -> str:
        if not window_ref:
            return "Sending to …"
        name = window_ref
        max_len = 28
        if len(name) > max_len:
            name = name[: max_len - 1] + "…"
        return f"Sending to {name}"

    def _build_mode_items(self) -> list:
        mode = self._active_mode()
        active_dot = "🟢 "
        inactive_dot = "⚪️ "

        relative = rumps.MenuItem(
            f"{active_dot if mode == 'RELATIVE' else inactive_dot}Relative",
            callback=self._menu_switch_relative,
        )
        absolute = rumps.MenuItem(
            f"{active_dot if mode == 'ABSOLUTE' else inactive_dot}Absolute",
            callback=self._menu_switch_absolute,
        )

        # The active mode is greyed out; only the inactive mode is selectable.
        if mode == "RELATIVE":
            relative.set_callback(None)
        elif mode == "ABSOLUTE":
            absolute.set_callback(None)
        else:
            relative.set_callback(None)
            absolute.set_callback(None)

        items: list = [relative, absolute]

        if mode == "ABSOLUTE":
            items.append(None)
            items.append(
                rumps.MenuItem(
                    self._sending_to_label(self._snapped_window_ref()),
                    callback=None,
                )
            )
            items.append(
                rumps.MenuItem("Choose window", callback=self._menu_choose_window)
            )

        return items

    def _menu_switch_relative(self, _=None) -> None:
        self._switch_to_relative()

    def _menu_switch_absolute(self, _=None) -> None:
        self._switch_to_absolute()

    def _menu_choose_window(self, _=None) -> None:
        self._restart_pick()

    def _make_connect_handler(self, connection_id: str):
        def handler(_):
            self.manager.toggle_connection(connection_id)
            self.refresh_menu()

        return handler

    def _update_overlay(self) -> None:
        conn = self._active_connection()
        connected = (
            conn is not None
            and self.manager.status(conn.id) == ConnectionStatus.CONNECTED
        )

        if not connected:
            if not self._picking:
                self.region_overlay.hide()
                self._stop_follow()
                self._snapped_conn_id = None
            return

        if conn.device_config.output_mode != "ABSOLUTE":
            self.region_overlay.hide()
            self._stop_follow()
            self._snapped_conn_id = None
            return

        if self._picking:
            return

        if self._snapped_conn_id != conn.id:
            self._start_pick(conn.id)
            return

        if (
            self._snapped_window_state != "minimized"
            and self.window_snap.snapped_lifecycle_state() != "minimized"
        ):
            self.region_overlay.show(conn.device_config.absolute)
        if not self._follow_timer.is_alive():
            self._follow_timer.start()
        self.connections_window.refresh_absolute_panel()

    def open_window(self, _=None) -> None:
        self.connections_window.show()

    def show_about(self, _=None) -> None:
        rumps.alert("About Reawa", ABOUT_MESSAGE)

    def quit_app(self, _=None) -> None:
        self.usb_watcher.stop()
        self.picker.stop()
        self.manager.disconnect()
        self.region_overlay.hide()
        rumps.quit_application()


def main() -> None:
    from remarkable.services.app_log import install_app_logging

    install_app_logging()
    RemarkableApp().run()


if __name__ == "__main__":
    main()
