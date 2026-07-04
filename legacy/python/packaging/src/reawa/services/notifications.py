from __future__ import annotations

import subprocess


class NotificationService:
    def send(self, title: str, message: str) -> None:
        try:
            script = f'display notification "{_escape(message)}" with title "{_escape(title)}"'
            subprocess.run(["osascript", "-e", script], check=False, capture_output=True)
        except Exception:
            pass


def _escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')
