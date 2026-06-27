from __future__ import annotations

from typing import Callable

import objc
from AppKit import (
    NSApplication,
    NSBackingStoreBuffered,
    NSBezierPath,
    NSBorderlessWindowMask,
    NSColor,
    NSFont,
    NSFontAttributeName,
    NSForegroundColorAttributeName,
    NSMakeRect,
    NSScreenSaverWindowLevel,
    NSView,
    NSWindow,
    NSWindowCollectionBehaviorCanJoinAllSpaces,
    NSWindowCollectionBehaviorFullScreenAuxiliary,
)
from Foundation import NSString, NSTimer
from Quartz import CGEventCreate, CGEventGetLocation

from remarkable.driver.window_snap import window_under_point
from remarkable.ui.display_bounds import cg_rect_to_cocoa

_ESCAPE_KEYCODE = 53


class SnapPickerView(NSView):
    def initWithOrigin_(self, origin):
        self = objc.super(SnapPickerView, self).init()
        if self is None:
            return None
        self.origin_x = origin[0]
        self.origin_y = origin[1]
        self.highlight = None  # cocoa rect tuple in view-local coords
        self.on_pick = None
        self.on_cancel = None
        self.show_hint = True
        return self

    def isOpaque(self):
        return False

    def acceptsFirstResponder(self):
        return True

    def drawRect_(self, rect):
        NSColor.colorWithWhite_alpha_(0.0, 0.28).set()
        NSBezierPath.fillRect_(self.bounds())
        if self.show_hint:
            self._draw_hint()

        if self.highlight is not None:
            hx, hy, hw, hh = self.highlight
            hole = NSMakeRect(hx, hy, hw, hh)
            NSColor.colorWithCalibratedRed_green_blue_alpha_(0.23, 0.51, 0.96, 0.22).set()
            NSBezierPath.fillRect_(hole)
            NSColor.colorWithCalibratedRed_green_blue_alpha_(0.23, 0.51, 0.96, 1.0).set()
            border = NSBezierPath.bezierPathWithRect_(hole)
            border.setLineWidth_(3.0)
            border.stroke()

    def _draw_hint(self):
        attrs = {
            NSForegroundColorAttributeName: NSColor.whiteColor(),
            NSFontAttributeName: NSFont.boldSystemFontOfSize_(15),
        }
        text = NSString.stringWithString_(
            "Click a window to snap the reMarkable region  ·  Esc to cancel"
        )
        size = text.sizeWithAttributes_(attrs)
        b = self.bounds()
        x = b.origin.x + (b.size.width - size.width) / 2.0
        y = b.origin.y + b.size.height - 64
        text.drawAtPoint_withAttributes_((x, y), attrs)

    def mouseDown_(self, event):
        loc = CGEventGetLocation(CGEventCreate(None))
        info = window_under_point(loc.x, loc.y)
        if info is not None and self.on_pick:
            self.on_pick(info)

    def keyDown_(self, event):
        if event.keyCode() == _ESCAPE_KEYCODE and self.on_cancel:
            self.on_cancel()


class _PickerWindow(NSWindow):
    def canBecomeKeyWindow(self):
        return True

    def constrainFrameRect_toScreen_(self, frameRect, screen):
        # AppKit otherwise shrinks/shifts borderless windows so they don't cover
        # the primary display's menu bar. On multi-display layouts that clips the
        # overlay off the primary screen, making windows there unclickable. We
        # span the full desktop intentionally, so keep the frame as-is.
        return frameRect


class _Ticker(objc.lookUpClass("NSObject")):
    def initWithCallback_(self, callback):
        self = objc.super(_Ticker, self).init()
        if self is None:
            return None
        self.callback = callback
        return self

    def fire_(self, timer):
        if self.callback:
            self.callback()


class SnapPicker:
    """Full-desktop overlay for choosing a window to snap the RM2 region to."""

    def __init__(self) -> None:
        self.windows: list = []
        self.views: list = []
        self.timer = None
        self._ticker = None
        self._on_pick = None
        self._on_cancel = None

    def start(
        self,
        on_pick: Callable[[tuple], None],
        on_cancel: Callable[[], None],
    ) -> None:
        from AppKit import NSScreen

        self._on_pick = on_pick
        self._on_cancel = on_cancel
        self.windows = []
        self.views = []

        # One overlay window per display. A single window spanning all displays
        # is only event-interactive on the display containing most of its area
        # (with "Displays have separate Spaces" on), so clicks on the other
        # displays never reach the view. Per-screen windows fix event routing.
        first = True
        for screen in NSScreen.screens():
            sf = screen.frame()
            frame = NSMakeRect(sf.origin.x, sf.origin.y, sf.size.width, sf.size.height)

            window = _PickerWindow.alloc().initWithContentRect_styleMask_backing_defer_(
                frame,
                NSBorderlessWindowMask,
                NSBackingStoreBuffered,
                False,
            )
            window.setLevel_(NSScreenSaverWindowLevel)
            window.setOpaque_(False)
            window.setBackgroundColor_(NSColor.clearColor())
            window.setIgnoresMouseEvents_(False)
            window.setCollectionBehavior_(
                NSWindowCollectionBehaviorCanJoinAllSpaces
                | NSWindowCollectionBehaviorFullScreenAuxiliary
            )

            view = SnapPickerView.alloc().initWithOrigin_((sf.origin.x, sf.origin.y))
            view.on_pick = self._handle_pick
            view.on_cancel = self._handle_cancel
            view.show_hint = first
            window.setContentView_(view)

            self.windows.append(window)
            self.views.append(view)
            first = False

        NSApplication.sharedApplication().activateIgnoringOtherApps_(True)
        for window in self.windows:
            window.orderFrontRegardless()
        if self.windows:
            self.windows[0].makeKeyAndOrderFront_(None)
            self.windows[0].makeFirstResponder_(self.views[0])

        self._ticker = _Ticker.alloc().initWithCallback_(self._poll)
        self.timer = NSTimer.scheduledTimerWithTimeInterval_target_selector_userInfo_repeats_(
            0.05, self._ticker, "fire:", None, True
        )

    def _poll(self) -> None:
        if not self.views:
            return
        loc = CGEventGetLocation(CGEventCreate(None))
        info = window_under_point(loc.x, loc.y)
        if info is None:
            for view in self.views:
                if view.highlight is not None:
                    view.highlight = None
                    view.setNeedsDisplay_(True)
            return
        _pid, _num, _name, (wx, wy, ww, wh) = info
        cx, cy, cw, ch = cg_rect_to_cocoa(wx, wy, ww, wh)
        for view in self.views:
            local = (cx - view.origin_x, cy - view.origin_y, cw, ch)
            if view.highlight != local:
                view.highlight = local
                view.setNeedsDisplay_(True)

    def _handle_pick(self, info) -> None:
        cb = self._on_pick
        self.stop()
        if cb:
            cb(info)

    def _handle_cancel(self) -> None:
        cb = self._on_cancel
        self.stop()
        if cb:
            cb()

    def stop(self) -> None:
        if self.timer is not None:
            self.timer.invalidate()
            self.timer = None
        self._ticker = None
        for window in self.windows:
            window.orderOut_(None)
        self.windows = []
        self.views = []
