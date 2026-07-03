from __future__ import annotations

import threading
from typing import Callable

from ..driver.session import DriverSession
from ..models.connection import Connection, ConnectionStatus
from ..models.store import ConnectionStore
from .keychain import KeychainStore


class ConnectionManager:
    def __init__(self) -> None:
        self.store = ConnectionStore()
        self._keychain: KeychainStore | None = None
        self._reachable: set[str] = set()
        self._discovered_ips: set[str] = set()
        self._errors: dict[str, str] = {}
        self._active_id: str | None = None
        self.active_session: DriverSession | None = None
        self._active_conn: Connection | None = None
        self._listeners: list[Callable[[], None]] = []
        self._lock = threading.Lock()

    @property
    def keychain(self) -> KeychainStore:
        if self._keychain is None:
            self._keychain = KeychainStore()
        return self._keychain

    def add_listener(self, callback: Callable[[], None]) -> None:
        self._listeners.append(callback)

    def _notify(self) -> None:
        for cb in self._listeners:
            try:
                cb()
            except Exception:
                pass

    def active_device_config(self):
        """Live device config for the active connection (in-memory, no file IO)."""
        if self._active_conn is not None:
            return self._active_conn.device_config
        return None

    def status(self, connection_id: str) -> ConnectionStatus:
        if (
            self._active_id == connection_id
            and self.active_session is not None
            and self.active_session.connected
            and self.active_session.is_running
        ):
            return ConnectionStatus.CONNECTED
        if connection_id in self._errors:
            return ConnectionStatus.ERROR
        if connection_id in self._reachable:
            return ConnectionStatus.ONLINE
        return ConnectionStatus.OFFLINE

    def error_message(self, connection_id: str) -> str | None:
        return self._errors.get(connection_id)

    def is_reachable(self, connection_id: str) -> bool:
        return connection_id in self._reachable

    def discovered_ips(self) -> set[str]:
        return set(self._discovered_ips)

    def set_discovered_ips(self, ips: set[str]) -> None:
        self._discovered_ips = set(ips)

    def active_connection_id(self) -> str | None:
        return self._active_id

    def list_connections(self) -> list[Connection]:
        return self.store.list_connections()

    def get_connection(self, connection_id: str) -> Connection | None:
        return self.store.get(connection_id)

    def update_reachability(self, reachable_ids: set[str]) -> None:
        with self._lock:
            changed = reachable_ids != self._reachable
            went_offline = self._reachable - reachable_ids
            self._reachable = set(reachable_ids)

            for conn_id in went_offline:
                self._errors.pop(conn_id, None)

            if changed:
                self._notify()

    def add_connection(
        self, name: str, ip: str, password: str, auto_connect: bool = False
    ) -> Connection:
        conn = Connection(name=name, ip=ip, auto_connect=auto_connect)
        self.store.add(conn)
        self.keychain.save_password(conn.id, password)

        key_path = self.store.private_key_path(conn.id)
        try:
            from ..driver.rm2 import setup_key

            setup_key(ip, password, key_path)
        except Exception as exc:
            self.store.remove(conn.id)
            self.keychain.delete_password(conn.id)
            raise RuntimeError(f"Failed to set up SSH key: {exc}") from exc

        self._notify()
        return conn

    def update_connection(self, connection: Connection) -> None:
        self.store.update(connection)
        if self._active_conn is not None and connection.id == self._active_conn.id:
            self._active_conn = connection
        self._notify()

    def remove_connection(self, connection_id: str) -> None:
        if self._active_id == connection_id:
            self.disconnect()
        self.store.remove(connection_id)
        self.keychain.delete_password(connection_id)
        self._reachable.discard(connection_id)
        self._errors.pop(connection_id, None)
        self._notify()

    def connect(self, connection_id: str) -> None:
        with self._lock:
            if (
                self._active_id == connection_id
                and self.active_session
                and self.active_session.connected
                and self.active_session.is_running
            ):
                return

            if self._active_id and self._active_id != connection_id:
                self._stop_session_unlocked()

            conn = self.store.get(connection_id)
            if not conn:
                raise KeyError(f"Unknown connection: {connection_id}")

            self._errors.pop(connection_id, None)
            self._active_conn = conn
            self._notify()

            key_path = self.store.private_key_path(connection_id)
            # Avoid keychain access on connect when the SSH key is already installed.
            # Password is only needed for first-time setup (add_connection) or if the
            # private key file is missing.
            password = None
            if not key_path.exists():
                password = self.keychain.get_password(connection_id)

            session = DriverSession(
                connection=conn,
                key_path=key_path,
                password=password,
                config_getter=self.active_device_config,
            )
            session.start()

            self.active_session = session
            self._active_id = connection_id
            threading.Timer(0.5, self._check_session, args=[connection_id, 0]).start()
            self._notify()

    def _check_session(self, connection_id: str, attempt: int = 0) -> None:
        session = self.active_session
        if session is None or self._active_id != connection_id:
            return

        if session.error:
            self._errors[connection_id] = session.error
            self._active_id = None
            self.active_session = None
            self._notify()
            return

        if session.connected and session.is_running:
            self._notify()
            return

        if not session.is_running:
            self._active_id = None
            self.active_session = None
            self._notify()
            return

        if attempt < 30:
            threading.Timer(
                0.5, self._check_session, args=[connection_id, attempt + 1]
            ).start()

    def _stop_session_unlocked(self) -> None:
        if self.active_session:
            self.active_session.stop()
            self.active_session = None
        self._active_id = None

    def pause_input(self) -> None:
        if self.active_session is not None:
            self.active_session.pause()

    def resume_input(self) -> None:
        if self.active_session is not None:
            self.active_session.resume()

    def disconnect(self) -> None:
        with self._lock:
            self._stop_session_unlocked()
            self._notify()

    def toggle_connection(self, connection_id: str) -> None:
        if (
            self._active_id == connection_id
            and self.active_session
            and self.active_session.connected
        ):
            self.disconnect()
        else:
            self.connect(connection_id)
