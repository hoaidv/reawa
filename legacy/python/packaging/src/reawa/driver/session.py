from __future__ import annotations

import socket
import sys
import threading
import traceback
from pathlib import Path
from typing import Callable

import paramiko

from . import rm2
from .absolute import AbsoluteDriver
from .mouse import MouseController
from .relative import RelativeDriver
from ..models.connection import Connection, DeviceConfig


class DriverSession:
    def __init__(
        self,
        connection: Connection,
        key_path: Path,
        password: str | None = None,
        on_region_change: Callable | None = None,
        config_getter: Callable[[], DeviceConfig] | None = None,
    ) -> None:
        self.connection = connection
        self.key_path = key_path
        self.password = password
        self.on_region_change = on_region_change
        self.config_getter = config_getter
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()
        self._paused = threading.Event()
        self._client: paramiko.SSHClient | None = None
        self._driver: RelativeDriver | AbsoluteDriver | None = None
        self.error: str | None = None
        self.connected = False

    def pause(self) -> None:
        """Stop translating pen frames to mouse input (stream stays open)."""
        self._paused.set()
        if self._driver is not None:
            self._driver.cleanup()

    def resume(self) -> None:
        self._paused.clear()

    @property
    def paused(self) -> bool:
        return self._paused.is_set()

    def _live_config(self) -> DeviceConfig:
        if self.config_getter is not None:
            cfg = self.config_getter()
            if cfg is not None:
                return cfg
        return self.connection.device_config

    def _make_driver(self, mode: str, mouse: MouseController):
        if mode == "ABSOLUTE":
            cfg = self._live_config()
            return AbsoluteDriver(mouse, cfg.absolute, on_region_change=self.on_region_change)
        return RelativeDriver(mouse)

    @property
    def is_running(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    def start(self) -> None:
        if self.is_running:
            return
        self._stop.clear()
        self.error = None
        self.connected = False
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._client is not None:
            try:
                self._client.close()
            except Exception:
                pass
        if self._thread is not None:
            self._thread.join(timeout=3)
            self._thread = None

    def _run(self) -> None:
        client = None
        try:
            print(
                f"[session] connecting to {self.connection.name} ({self.connection.ip})...",
                file=sys.stderr,
            )
            client, stdout = rm2.connect_pen_stream(
                ip=self.connection.ip,
                key_path=self.key_path,
                password=self.password,
            )
            self._client = client
            self.connected = True
            print(f"[session] connected to {self.connection.name}", file=sys.stderr)

            cfg = self._live_config()
            mouse = MouseController(cfg)
            current_mode = cfg.output_mode
            driver = self._make_driver(current_mode, mouse)
            self._driver = driver

            for frame in rm2.read_pen_frames(stdout):
                if self._stop.is_set():
                    break
                if self._paused.is_set():
                    continue

                cfg = self._live_config()
                mouse.config = cfg

                # Swap the driver live when the output mode changes (e.g. the
                # user snaps a window or releases back to RELATIVE) so we never
                # need to tear down and rebuild the SSH session.
                if cfg.output_mode != current_mode:
                    driver.cleanup()
                    current_mode = cfg.output_mode
                    driver = self._make_driver(current_mode, mouse)
                    self._driver = driver

                if isinstance(driver, AbsoluteDriver):
                    driver.update_region(cfg.absolute)
                driver.handle_frame(frame)

        except Exception as exc:
            self.error = str(exc) or exc.__class__.__name__
            self.connected = False
            print(f"[session] error: {self.error}", file=sys.stderr)
            traceback.print_exc()
        finally:
            if self._driver is not None:
                self._driver.cleanup()
            if client is not None:
                try:
                    client.close()
                except Exception:
                    pass
            self._client = None
            self._driver = None


def is_host_reachable(ip: str, port: int = 22, timeout: float = 1.0) -> bool:
    try:
        with socket.create_connection((ip, port), timeout=timeout):
            return True
    except OSError:
        return False
