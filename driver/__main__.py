#!/usr/bin/env python3
"""CLI entry point for the reMarkable pen driver."""

from __future__ import annotations

import sys
import time

from remarkable.models.store import ConnectionStore
from remarkable.services.connection_manager import ConnectionManager


def main() -> None:
    store = ConnectionStore()
    connections = store.list_connections()
    if not connections:
        print(
            "No connections configured. Run the menubar app: python -m remarkable",
            file=sys.stderr,
        )
        sys.exit(1)

    conn = connections[0]
    manager = ConnectionManager()
    manager.add_listener(lambda: None)

    try:
        manager.connect(conn.id)
        print(f"Connected to {conn.name}. Press Ctrl+C to stop.")
        while manager.active_session and manager.active_session.is_running:
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        manager.disconnect()


if __name__ == "__main__":
    main()
