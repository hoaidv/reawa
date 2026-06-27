from __future__ import annotations

import threading
import time
from typing import Callable

from remarkable.driver.session import is_host_reachable
from remarkable.models.connection import Connection
from remarkable.services.connection_manager import ConnectionManager
from remarkable.services.network_discovery import discover_ssh_hosts, discover_usb_ssh_hosts
from remarkable.services.notifications import NotificationService


class USBWatcher:
    def __init__(
        self,
        manager: ConnectionManager,
        notifications: NotificationService,
        interval: float = 3.0,
    ) -> None:
        self.manager = manager
        self.notifications = notifications
        self.interval = interval
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()
        self._was_reachable: set[str] = set()
        self._was_discovered_ips: set[str] = set()
        self._on_detected: Callable[[Connection], None] | None = None

    def set_on_detected(self, callback: Callable[[Connection], None]) -> None:
        self._on_detected = callback

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    def _loop(self) -> None:
        while not self._stop.is_set():
            self._poll()
            time.sleep(self.interval)

    def _discovered_ips(self) -> set[str]:
        """Scan USB/local subnets for SSH without using saved connection IPs."""
        try:
            ips = discover_usb_ssh_hosts()
            if ips:
                return ips
            return discover_ssh_hosts()
        except Exception:
            return set()

    def _connection_matches_discovery(self, conn: Connection, discovered: set[str]) -> bool:
        if conn.ip in discovered:
            return True
        if is_host_reachable(conn.ip):
            return True
        return False

    def _poll(self) -> None:
        connections = self.manager.list_connections()
        discovered = self._discovered_ips()
        self.manager.set_discovered_ips(discovered)

        currently: set[str] = set()
        for conn in connections:
            if self._connection_matches_discovery(conn, discovered):
                currently.add(conn.id)

        self.manager.update_reachability(currently)

        new_ips = discovered - self._was_discovered_ips
        if new_ips and not connections:
            ip_list = ", ".join(sorted(new_ips))
            self.notifications.send(
                "Reawa Detected",
                f"SSH device at {ip_list} — add a connection in Open",
            )

        newly_online = currently - self._was_reachable
        for conn_id in newly_online:
            conn = self.manager.get_connection(conn_id)
            if not conn:
                continue
            if conn.auto_connect:
                try:
                    self.manager.connect(conn.id)
                    self.notifications.send(
                        "Reawa Connected",
                        f"Auto-connected to {conn.name}",
                    )
                except Exception as exc:
                    self.notifications.send(
                        "Reawa Connection Failed",
                        f"{conn.name}: {exc}",
                    )
            else:
                self.notifications.send(
                    "Reawa Detected",
                    f"{conn.name} is available — open the app to connect",
                )
                if self._on_detected:
                    self._on_detected(conn)

        went_offline = self._was_reachable - currently
        active = self.manager.active_connection_id()
        if active and active in went_offline:
            self.manager.disconnect()
            conn = self.manager.get_connection(active)
            name = conn.name if conn else active
            self.notifications.send("Reawa Disconnected", f"{name} is no longer reachable")

        self._was_reachable = currently
        self._was_discovered_ips = discovered
