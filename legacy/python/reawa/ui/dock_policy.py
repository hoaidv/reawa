from __future__ import annotations

from AppKit import (
    NSApplication,
    NSApplicationActivationPolicyAccessory,
    NSApplicationActivationPolicyRegular,
)


def set_dock_visible(visible: bool) -> None:
    """Show or hide the Dock icon (menubar-only when hidden)."""
    # sharedApplication() creates the NSApplication if it doesn't exist yet;
    # NSApp() would return None before rumps starts the app loop.
    app = NSApplication.sharedApplication()
    if visible:
        app.setActivationPolicy_(NSApplicationActivationPolicyRegular)
        app.activateIgnoringOtherApps_(True)
    else:
        app.setActivationPolicy_(NSApplicationActivationPolicyAccessory)
