from __future__ import annotations

import os
import time

import Quartz
from ApplicationServices import (
    AXUIElementCopyAttributeValue,
    AXUIElementCreateApplication,
    AXUIElementPerformAction,
    AXUIElementSetAttributeValue,
    AXValueCreate,
    AXValueGetValue,
    kAXErrorInvalidUIElement,
    kAXErrorSuccess,
    kAXMinimizedAttribute,
    kAXPositionAttribute,
    kAXRaiseAction,
    kAXRoleAttribute,
    kAXSizeAttribute,
    kAXTitleAttribute,
    kAXValueCGPointType,
    kAXValueCGSizeType,
    kAXWindowsAttribute,
)
from AppKit import NSApplicationActivateAllWindows, NSRunningApplication, NSScreen
from CoreFoundation import CGPoint, CGSize

from . import rm2
from ..models.connection import AbsoluteConfig
from ..ui.display_bounds import primary_height

# pid, window_number, title, (x, y, w, h) in Quartz top-left global coordinates
WindowInfo = tuple[int, int, str, tuple[float, float, float, float]]

WindowLifecycle = str  # "closed" | "minimized" | "maximized" | "normal"

_FRAME_TOLERANCE = 6.0

# A window swept into the Stage Manager strip reports a CGWindow thumbnail far
# smaller than its (still full-size) AX frame. Treat it as staged when the live
# on-screen area drops below this fraction of the AX frame area.
_STAGE_AREA_RATIO = 0.5

# kAXFullScreenAttribute is not exported by every PyObjC build, but AX accepts
# the raw attribute-name string just as well.
_AX_FULLSCREEN_ATTR = "AXFullScreen"


def _ax_copy(element, attr):
    err, value = AXUIElementCopyAttributeValue(element, attr, None)
    if err != kAXErrorSuccess:
        return None
    return value


def _unwrap_point(value):
    """Extract a CGPoint from an AXValueRef (or pass through a CGPoint)."""
    if hasattr(value, "x"):
        return value
    ok, pt = AXValueGetValue(value, kAXValueCGPointType, None)
    return pt if ok else None


def _unwrap_size(value):
    """Extract a CGSize from an AXValueRef (or pass through a CGSize)."""
    if hasattr(value, "width"):
        return value
    ok, sz = AXValueGetValue(value, kAXValueCGSizeType, None)
    return sz if ok else None


def _ax_bool(value) -> bool | None:
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    try:
        return bool(int(value))
    except (TypeError, ValueError):
        return bool(value)


def window_cg_onscreen_bounds(
    number: int,
) -> tuple[float, float, float, float] | None:
    """The window's live on-screen bounds (Quartz) from CGWindowList, or None.

    Distinct from the AX frame: when a window is swept into the Stage Manager
    strip, its CGWindow bounds shrink to the thumbnail size while its AX frame
    stays full-size. That mismatch is the reliable "staged" signal.
    """
    if number <= 0:
        return None
    raw = (
        Quartz.CGWindowListCopyWindowInfo(
            Quartz.kCGWindowListOptionOnScreenOnly
            | Quartz.kCGWindowListExcludeDesktopElements,
            Quartz.kCGNullWindowID,
        )
        or []
    )
    for e in raw:
        if int(e.get("kCGWindowNumber", 0)) == number:
            b = e.get("kCGWindowBounds") or {}
            return (
                float(b.get("X", 0)),
                float(b.get("Y", 0)),
                float(b.get("Width", 0)),
                float(b.get("Height", 0)),
            )
    return None


def visible_frame_quartz_at(x: float, y: float) -> tuple[float, float, float, float]:
    """Visible desktop area (menu bar / dock excluded) in Quartz coords for *x*, *y*."""
    ph = primary_height()
    screens = NSScreen.screens() or [NSScreen.mainScreen()]
    for screen in screens:
        vf = screen.visibleFrame()
        qx = float(vf.origin.x)
        qw = float(vf.size.width)
        qh = float(vf.size.height)
        qy = ph - float(vf.origin.y) - qh
        if qx <= x <= qx + qw and qy <= y <= qy + qh:
            return qx, qy, qw, qh
    vf = screens[0].visibleFrame()
    return (
        float(vf.origin.x),
        ph - float(vf.origin.y) - float(vf.size.height),
        float(vf.size.width),
        float(vf.size.height),
    )


def _frame_near_visible_maximize(wx: float, wy: float, ww: float, wh: float) -> bool:
    cx = wx + ww / 2.0
    cy = wy + wh / 2.0
    vx, vy, vw, vh = visible_frame_quartz_at(cx, cy)
    return (
        abs(wx - vx) <= _FRAME_TOLERANCE
        and abs(wy - vy) <= _FRAME_TOLERANCE
        and abs(ww - vw) <= _FRAME_TOLERANCE
        and abs(wh - vh) <= _FRAME_TOLERANCE
    )


def window_frame(window) -> tuple[float, float, float, float] | None:
    pos = _ax_copy(window, kAXPositionAttribute)
    size = _ax_copy(window, kAXSizeAttribute)
    if pos is None or size is None:
        return None
    pt = _unwrap_point(pos)
    sz = _unwrap_size(size)
    if pt is None or sz is None:
        return None
    return float(pt.x), float(pt.y), float(sz.width), float(sz.height)


def list_onscreen_windows() -> list[WindowInfo]:
    """On-screen app windows front-to-back, in Quartz top-left global coordinates."""
    options = (
        Quartz.kCGWindowListOptionOnScreenOnly
        | Quartz.kCGWindowListExcludeDesktopElements
    )
    raw = Quartz.CGWindowListCopyWindowInfo(options, Quartz.kCGNullWindowID) or []
    me = os.getpid()
    windows: list[WindowInfo] = []
    for entry in raw:
        if int(entry.get("kCGWindowLayer", 0)) != 0:
            continue
        pid = int(entry.get("kCGWindowOwnerPID", 0))
        if pid == me:
            continue
        bounds = entry.get("kCGWindowBounds")
        if not bounds:
            continue
        x = float(bounds["X"])
        y = float(bounds["Y"])
        w = float(bounds["Width"])
        h = float(bounds["Height"])
        if w < 40 or h < 40:
            continue
        name = entry.get("kCGWindowName") or entry.get("kCGWindowOwnerName") or ""
        windows.append((pid, int(entry.get("kCGWindowNumber", 0)), str(name), (x, y, w, h)))
    return windows


def window_under_point(x: float, y: float) -> WindowInfo | None:
    """Hit-test in Quartz top-left coordinates (matches CGEventGetLocation)."""
    for info in list_onscreen_windows():
        _pid, _num, _name, (wx, wy, ww, wh) = info
        if wx <= x <= wx + ww and wy <= y <= wy + wh:
            return info
    return None


def _layer0_window_numbers(option) -> set[int]:
    raw = (
        Quartz.CGWindowListCopyWindowInfo(
            option | Quartz.kCGWindowListExcludeDesktopElements,
            Quartz.kCGNullWindowID,
        )
        or []
    )
    return {
        int(entry.get("kCGWindowNumber", 0))
        for entry in raw
        if int(entry.get("kCGWindowLayer", 0)) == 0
    }


def window_is_onscreen(window_number: int) -> bool:
    """True if the window is currently on a visible screen/space (i.e. not minimized).

    Membership in the *OnScreenOnly* list is the signal — the per-window
    ``kCGWindowIsOnscreen`` flag is unreliable (frequently absent under Stage
    Manager). A minimized window, a Cmd+H–hidden window, and a window on another
    Stage Manager stage / Space are all absent from this list.

    Note: this is **not** a close check — closing is detected separately via AX
    element validity, because a Stage-Manager-minimized window leaves *every*
    ``CGWindowList`` query yet its window still exists.
    """
    if window_number <= 0:
        return True  # unknown number — don't claim it's off-screen
    return window_number in _layer0_window_numbers(Quartz.kCGWindowListOptionOnScreenOnly)


def _resolve_ax_window(pid: int, x: float, y: float, w: float, h: float):
    app_el = AXUIElementCreateApplication(pid)
    err, windows = AXUIElementCopyAttributeValue(app_el, kAXWindowsAttribute, None)
    if err != kAXErrorSuccess or not windows:
        return None
    best = None
    best_dist = float("inf")
    for window in windows:
        frame = window_frame(window)
        if not frame:
            continue
        wx, wy, ww, wh = frame
        dist = abs(wx - x) + abs(wy - y) + abs(ww - w) + abs(wh - h)
        if dist < best_dist:
            best_dist = dist
            best = window
    return best


class WindowSnapController:
    def __init__(self) -> None:
        self._snapped_window = None
        self._snapped_ref: str | None = None
        self._snapped_number: int = 0
        self._snapped_pid: int = 0

    @property
    def snapped_ref(self) -> str | None:
        return self._snapped_ref

    def has_window(self) -> bool:
        return self._snapped_window is not None

    def clear(self) -> None:
        self._snapped_window = None
        self._snapped_ref = None
        self._snapped_number = 0
        self._snapped_pid = 0

    def _is_stage_manager_minimized(self) -> bool:
        """True when the snapped window was swept into the Stage Manager strip.

        Stage Manager does not set ``kAXMinimizedAttribute`` and the window can
        remain in the on-screen CGWindowList. The reliable signal is that the
        window's live CGWindow on-screen bounds shrink to a thumbnail while its
        AX frame stays full-size (area ratio well below 1).
        """
        if self._snapped_window is None:
            return False
        ax_frame = window_frame(self._snapped_window)
        cg_bounds = window_cg_onscreen_bounds(self._snapped_number)
        if ax_frame is None or cg_bounds is None:
            return False
        ax_area = ax_frame[2] * ax_frame[3]
        if ax_area <= 0:
            return False
        cg_area = cg_bounds[2] * cg_bounds[3]
        return (cg_area / ax_area) < _STAGE_AREA_RATIO

    def pick_from_info(self, info: WindowInfo) -> tuple[str, float, float, float, float]:
        pid, num, name, (x, y, w, h) = info
        self._snapped_window = _resolve_ax_window(pid, x, y, w, h)
        self._snapped_ref = name or f"pid {pid}"
        self._snapped_number = int(num)
        self._snapped_pid = int(pid)
        return self._snapped_ref, x, y, w, h

    def _element_alive(self) -> bool:
        """False only when AX reports the element is invalid (window closed).

        A merely minimized window (including Stage Manager) keeps a valid AX
        element, so this is the authoritative close signal — unlike CGWindowList,
        which drops Stage-Manager-minimized windows even though they still exist.
        Transient errors (e.g. app busy => kAXErrorCannotComplete) are treated as
        alive to avoid spuriously reverting to RELATIVE.
        """
        if self._snapped_window is None:
            return False
        err, _ = AXUIElementCopyAttributeValue(
            self._snapped_window, kAXRoleAttribute, None
        )
        return err != kAXErrorInvalidUIElement

    def is_window_closed(self) -> bool:
        """True if the snapped window no longer exists (closed, not minimized)."""
        return not self._element_alive()

    def restore_window(self) -> tuple[float, float, float, float] | None:
        """Un-minimize / un-stage the snapped window and bring it forward.

        Returns the settled (post-restore) window frame so the caller can snap to
        the real bounds rather than a tiny Stage Manager thumbnail. Blocks briefly
        (~0.5 s max) while the window animates back to full size.
        """
        if self._snapped_window is None:
            return None
        AXUIElementSetAttributeValue(self._snapped_window, kAXMinimizedAttribute, False)
        AXUIElementPerformAction(self._snapped_window, kAXRaiseAction)
        if self._snapped_pid:
            app = NSRunningApplication.runningApplicationWithProcessIdentifier_(
                self._snapped_pid
            )
            if app is not None:
                app.activateWithOptions_(NSApplicationActivateAllWindows)
        return self._await_stable_frame()

    def _await_stable_frame(self) -> tuple[float, float, float, float] | None:
        """Poll the window frame until it stops changing (restore animation done)."""
        last: tuple[float, float, float, float] | None = None
        for _ in range(10):  # up to ~0.5 s
            frame = self.current_window_frame()
            if (
                frame is not None
                and last is not None
                and abs(frame[2] - last[2]) < 2
                and abs(frame[3] - last[3]) < 2
            ):
                return frame
            last = frame
            time.sleep(0.05)
        return last

    def pick_window_at_point(
        self, x: float, y: float
    ) -> tuple[str, float, float, float, float] | None:
        info = window_under_point(x, y)
        if info is None:
            return None
        return self.pick_from_info(info)

    def snap_region_to_window(
        self,
        region: AbsoluteConfig,
        window_x: float,
        window_y: float,
        window_w: float,
        window_h: float,
    ) -> AbsoluteConfig:
        target_h = window_w / rm2.RM2_ASPECT
        if target_h > window_h:
            target_w = window_h * rm2.RM2_ASPECT
        else:
            target_w = window_w
        region.region_x = window_x
        region.region_y = window_y
        region.region_width = target_w
        region.lock_aspect()
        return region

    def current_window_frame(self) -> tuple[float, float, float, float] | None:
        if self._snapped_window is None:
            return None
        return window_frame(self._snapped_window)

    def snapped_lifecycle_state(self) -> WindowLifecycle:
        """Lifecycle of the bound AX window: closed, minimized, maximized, or normal."""
        if self._snapped_window is None:
            return "closed"

        if not self._element_alive():
            return "closed"

        if _ax_bool(_ax_copy(self._snapped_window, kAXMinimizedAttribute)):
            return "minimized"
        if not window_is_onscreen(self._snapped_number):
            return "minimized"
        if self._is_stage_manager_minimized():
            return "minimized"

        frame = window_frame(self._snapped_window)
        if frame is None:
            return "normal"

        fullscreen = _ax_bool(_ax_copy(self._snapped_window, _AX_FULLSCREEN_ATTR))
        if fullscreen or _frame_near_visible_maximize(*frame):
            return "maximized"
        return "normal"

    def sync_region_to_window(self, region: AbsoluteConfig) -> bool:
        """Align *region* to the snapped window frame (aspect-fit inside). Returns False if gone."""
        frame = self.current_window_frame()
        if frame is None:
            return False
        wx, wy, ww, wh = frame
        self.snap_region_to_window(region, wx, wy, ww, wh)
        return True

    def sync_window_to_region(self, region: AbsoluteConfig) -> None:
        if self._snapped_window is None:
            return
        # AX position/size attributes must be set with an AXValueRef, not a raw
        # CGPoint/CGSize — passing the bare struct is silently ignored, so the
        # window never moves or resizes. Set size first, then position, so a
        # window being shrunk doesn't get clamped by its current (larger) frame
        # when we move it.
        pos = AXValueCreate(kAXValueCGPointType, CGPoint(region.region_x, region.region_y))
        size = AXValueCreate(kAXValueCGSizeType, CGSize(region.region_width, region.region_height))
        AXUIElementSetAttributeValue(self._snapped_window, kAXSizeAttribute, size)
        AXUIElementSetAttributeValue(self._snapped_window, kAXPositionAttribute, pos)
        AXUIElementSetAttributeValue(self._snapped_window, kAXSizeAttribute, size)
