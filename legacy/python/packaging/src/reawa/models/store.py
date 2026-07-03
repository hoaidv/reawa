from __future__ import annotations

import json
import shutil
from pathlib import Path
from typing import Iterable

from .connection import Connection

APP_SUPPORT = Path.home() / "Library" / "Application Support" / "remarkable-rm2"
CONNECTIONS_FILE = APP_SUPPORT / "connections.json"
KEYS_DIR = APP_SUPPORT / "keys"

# Legacy paths (project-local, pre-menubar)
LEGACY_CONFIG = Path(__file__).resolve().parent.parent / ".rm2_config.json"
LEGACY_KEY = Path(__file__).resolve().parent.parent / ".ssh" / "id_rsa"


class ConnectionStore:
    def __init__(self) -> None:
        APP_SUPPORT.mkdir(parents=True, exist_ok=True)
        KEYS_DIR.mkdir(parents=True, exist_ok=True)
        self._migrate_legacy_if_needed()

    def key_dir(self, connection_id: str) -> Path:
        return KEYS_DIR / connection_id

    def private_key_path(self, connection_id: str) -> Path:
        return self.key_dir(connection_id) / "id_rsa"

    def public_key_path(self, connection_id: str) -> Path:
        return self.key_dir(connection_id) / "id_rsa.pub"

    def list_connections(self) -> list[Connection]:
        if not CONNECTIONS_FILE.exists():
            return []
        data = json.loads(CONNECTIONS_FILE.read_text())
        return [Connection.from_dict(item) for item in data.get("connections", [])]

    def save_connections(self, connections: Iterable[Connection]) -> None:
        payload = {"connections": [c.to_dict() for c in connections]}
        CONNECTIONS_FILE.write_text(json.dumps(payload, indent=2) + "\n")

    def get(self, connection_id: str) -> Connection | None:
        for conn in self.list_connections():
            if conn.id == connection_id:
                return conn
        return None

    def add(self, connection: Connection) -> None:
        connections = self.list_connections()
        connections.append(connection)
        self.save_connections(connections)

    def update(self, connection: Connection) -> None:
        connections = self.list_connections()
        for i, existing in enumerate(connections):
            if existing.id == connection.id:
                connections[i] = connection
                self.save_connections(connections)
                return
        raise KeyError(f"Connection not found: {connection.id}")

    def remove(self, connection_id: str) -> None:
        connections = [c for c in self.list_connections() if c.id != connection_id]
        self.save_connections(connections)
        key_dir = self.key_dir(connection_id)
        if key_dir.exists():
            shutil.rmtree(key_dir)

    def _migrate_legacy_if_needed(self) -> None:
        if CONNECTIONS_FILE.exists():
            return
        if not LEGACY_CONFIG.exists() or not LEGACY_KEY.exists():
            return

        legacy = json.loads(LEGACY_CONFIG.read_text())
        ip = legacy.get("ip", "10.11.99.1")
        conn = Connection(name="My reMarkable", ip=ip, auto_connect=True)
        self.add(conn)

        dest_dir = self.key_dir(conn.id)
        dest_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(LEGACY_KEY, self.private_key_path(conn.id))
        pub = LEGACY_KEY.with_name("id_rsa.pub")
        if pub.exists():
            shutil.copy2(pub, self.public_key_path(conn.id))
