from __future__ import annotations

import sys
import threading
from collections import deque
from dataclasses import dataclass
from datetime import datetime
from typing import Callable

MAX_ENTRIES = 2000


@dataclass(frozen=True)
class LogEntry:
    timestamp: datetime
    stream: str
    message: str

    def format_line(self) -> str:
        ms = self.timestamp.microsecond // 1000
        ts = self.timestamp.strftime("%Y-%m-%d %H:%M:%S") + f".{ms:03d}"
        return f"{ts}  {self.message}"


class AppLog:
    def __init__(self) -> None:
        self._entries: deque[LogEntry] = deque(maxlen=MAX_ENTRIES)
        self._lock = threading.Lock()
        self._listeners: list[Callable[[], None]] = []

    def record(self, message: str, stream: str = "log") -> None:
        text = message.strip()
        if not text:
            return
        entry = LogEntry(timestamp=datetime.now(), stream=stream, message=text)
        with self._lock:
            self._entries.append(entry)
        self._notify()

    def entries(self) -> list[LogEntry]:
        with self._lock:
            return list(self._entries)

    def add_listener(self, callback: Callable[[], None]) -> None:
        self._listeners.append(callback)

    def remove_listener(self, callback: Callable[[], None]) -> None:
        try:
            self._listeners.remove(callback)
        except ValueError:
            pass

    def _notify(self) -> None:
        for callback in list(self._listeners):
            try:
                callback()
            except Exception:
                pass


app_log = AppLog()
_installed = False


class _TeeIO:
    def __init__(self, stream_name: str, original) -> None:
        self.stream_name = stream_name
        self._original = original
        self._partial = ""

    def write(self, s: str) -> int:
        if self._original is not None:
            self._original.write(s)
        if s:
            text = self._partial + s
            lines = text.split("\n")
            if text.endswith("\n"):
                self._partial = ""
            else:
                self._partial = lines.pop() if lines else text
            for line in lines:
                app_log.record(line, self.stream_name)
        return len(s)

    def flush(self) -> None:
        if self._original is not None:
            self._original.flush()
        if self._partial:
            app_log.record(self._partial, self.stream_name)
            self._partial = ""

    def fileno(self) -> int:
        if self._original is None:
            raise OSError("No underlying stream")
        return self._original.fileno()

    def isatty(self) -> bool:
        if self._original is None:
            return False
        return self._original.isatty()


def install_app_logging() -> None:
    global _installed
    if _installed:
        return
    _installed = True
    sys.stdout = _TeeIO("stdout", sys.stdout)
    sys.stderr = _TeeIO("stderr", sys.stderr)
    app_log.record("Reawa started", "info")
