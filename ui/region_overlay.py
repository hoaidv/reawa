from __future__ import annotations

import objc
from AppKit import (
    NSBackingStoreBuffered,
    NSBezierPath,
    NSBorderlessWindowMask,
    NSColor,
    NSCursor,
    NSFloatingWindowLevel,
    NSMakeRect,
    NSScreenSaverWindowLevel,
    NSTrackingActiveAlways,
    NSTrackingArea,
    NSTrackingCursorUpdate,
    NSTrackingMouseEnteredAndExited,
    NSView,
    NSWindow,
    NSWindowCollectionBehaviorCanJoinAllSpaces,
    NSWindowCollectionBehaviorFullScreenAuxiliary,
)
from Quartz import CGEventCreate, CGEventGetLocation

from remarkable.models.connection import RM2_ASPECT, AbsoluteConfig
from remarkable.ui.display_bounds import (
    cg_point_to_cocoa,
    cg_rect_to_cocoa,
    desktop_bounds,
)

_HANDLE = 18.0
_MIN_W = 160.0


def _diagonal_cursor(nwse: bool) -> NSCursor:
    sel = (
        "_windowResizeNorthWestSouthEastCursor"
        if nwse
        else "_windowResizeNorthEastSouthWestCursor"
    )
    try:
        cursor = NSCursor.performSelector_(sel)
        if cursor is not None:
            return cursor
    except Exception:
        pass
    return NSCursor.crosshairCursor()


class _OverlayWindow(NSWindow):
    def constrainFrameRect_toScreen_(self, frameRect, screen):
        # Span the full multi-display desktop without AppKit clipping the frame
        # off the primary display to avoid the menu bar (see snap_picker).
        return frameRect


class RegionOverlayView(NSView):
    def initWithOrigin_(self, origin):
        self = objc.super(RegionOverlayView, self).init()
        if self is None:
            return None
        self.origin_x = origin[0]
        self.origin_y = origin[1]
        self.cocoa_hole = None  # (x, y, w, h) local
        self.border_color = "#3B82F6"
        self.border_style = "solid"
        return self

    def isOpaque(self):
        return False

    def drawRect_(self, rect):
        # Border only: the desktop outside the region is left fully visible and
        # interactive. We draw nothing but a stroked outline around the region.
        if self.cocoa_hole is None:
            return
        hx, hy, hw, hh = self.cocoa_hole
        border = NSColor.colorWithCalibratedRed_green_blue_alpha_(
            *_hex_to_rgb(self.border_color), 1.0
        )
        border.set()
        stroke = NSBezierPath.bezierPathWithRect_(NSMakeRect(hx, hy, hw, hh))
        stroke.setLineWidth_(3.0)
        if self.border_style == "dashed":
            stroke.setLineDash_count_phase_([8, 6], 2, 0)
        stroke.stroke()


class RegionHandleView(NSView):
    def initWithCorner_controller_(self, corner, controller):
        self = objc.super(RegionHandleView, self).init()
        if self is None:
            return None
        self.corner = corner  # "tl" | "tr" | "bl" | "br"
        self.controller = controller
        self._tracking = None
        return self

    def _cursor(self) -> NSCursor:
        return _diagonal_cursor(self.corner in ("tl", "br"))

    def updateTrackingAreas(self):
        if self._tracking is not None:
            self.removeTrackingArea_(self._tracking)
        self._tracking = NSTrackingArea.alloc().initWithRect_options_owner_userInfo_(
            self.bounds(),
            NSTrackingMouseEnteredAndExited
            | NSTrackingActiveAlways
            | NSTrackingCursorUpdate,
            self,
            None,
        )
        self.addTrackingArea_(self._tracking)

    def resetCursorRects(self):
        self.addCursorRect_cursor_(self.bounds(), self._cursor())

    def cursorUpdate_(self, event):
        self._cursor().set()

    def mouseEntered_(self, event):
        self._cursor().set()

    def drawRect_(self, rect):
        b = self.bounds()
        NSColor.colorWithCalibratedRed_green_blue_alpha_(0.23, 0.51, 0.96, 1.0).set()
        dot = NSBezierPath.bezierPathWithOvalInRect_(
            NSMakeRect(b.size.width / 2 - 5, b.size.height / 2 - 5, 10, 10)
        )
        dot.fill()
        NSColor.whiteColor().set()
        dot.setLineWidth_(1.5)
        dot.stroke()

    def mouseDown_(self, event):
        self._cursor().set()

    def mouseDragged_(self, event):
        loc = CGEventGetLocation(CGEventCreate(None))
        self.controller.resize_from_corner(self.corner, loc.x, loc.y)


class RegionOverlayController:
    """Click-through region overlay plus corner resize handles.

    The region is stored in Quartz global coordinates (top-left origin) so it
    matches synthesized mouse events and Accessibility window frames; it is
    converted to Cocoa coordinates only for drawing. Mode switching and window
    re-snapping are driven from the menu bar, not an on-screen toolbar.
    """

    def __init__(self, callback) -> None:
        self.callback = callback
        self.region = AbsoluteConfig()
        self.window = None
        self.view = None
        self._origin = (0.0, 0.0)
        self._handles: dict[str, NSWindow] = {}

    def show(self, region: AbsoluteConfig) -> None:
        self.region = region
        min_x, min_y, max_x, max_y = desktop_bounds()
        self._origin = (min_x, min_y)
        frame = NSMakeRect(min_x, min_y, max_x - min_x, max_y - min_y)

        if self.window is None:
            self.window = _OverlayWindow.alloc().initWithContentRect_styleMask_backing_defer_(
                frame, NSBorderlessWindowMask, NSBackingStoreBuffered, False
            )
            self.window.setLevel_(NSFloatingWindowLevel)
            self.window.setOpaque_(False)
            self.window.setBackgroundColor_(NSColor.clearColor())
            self.window.setIgnoresMouseEvents_(True)  # pen + real mouse pass through
            self.window.setCollectionBehavior_(
                NSWindowCollectionBehaviorCanJoinAllSpaces
                | NSWindowCollectionBehaviorFullScreenAuxiliary
            )
            self.view = RegionOverlayView.alloc().initWithOrigin_((min_x, min_y))
            self.window.setContentView_(self.view)
        else:
            self.window.setFrame_display_(frame, False)
            self.view.origin_x = min_x
            self.view.origin_y = min_y

        self._build_handles()
        self._refresh()
        self.window.orderFrontRegardless()

    def hide(self) -> None:
        if self.window:
            self.window.orderOut_(None)
        for win in self._handles.values():
            win.orderOut_(None)

    def update_region(self, region: AbsoluteConfig) -> None:
        self.region = region
        self._refresh()

    def resize_from_corner(self, corner: str, px: float, py: float) -> None:
        # px, py are Quartz top-left coords (CGEventGetLocation); region is too.
        # The corner opposite the dragged one stays fixed.
        r = self.region
        x, y, w, h = r.region_x, r.region_y, r.region_width, r.region_height
        if corner == "tl":
            anchor_x, anchor_y = x + w, y + h  # br fixed
        elif corner == "tr":
            anchor_x, anchor_y = x, y + h       # bl fixed
        elif corner == "bl":
            anchor_x, anchor_y = x + w, y        # tr fixed
        else:  # br
            anchor_x, anchor_y = x, y            # tl fixed

        new_w = max(_MIN_W, abs(anchor_x - px))
        new_h = new_w / RM2_ASPECT
        new_x = anchor_x - new_w if px < anchor_x else anchor_x
        new_y = anchor_y - new_h if py < anchor_y else anchor_y

        r.region_x = new_x
        r.region_y = new_y
        r.region_width = new_w
        r.lock_aspect()
        self._refresh()
        if self.callback:
            self.callback(r)

    def _cocoa_hole(self) -> tuple[float, float, float, float]:
        cx, cy, cw, ch = cg_rect_to_cocoa(
            self.region.region_x,
            self.region.region_y,
            self.region.region_width,
            self.region.region_height,
        )
        return cx - self._origin[0], cy - self._origin[1], cw, ch

    def _refresh(self) -> None:
        if self.view is not None:
            self.view.cocoa_hole = self._cocoa_hole()
            self.view.border_color = self.region.border_color
            self.view.border_style = self.region.border_style
            self.view.setNeedsDisplay_(True)
        self._position_handles()

    def _build_handles(self) -> None:
        if self._handles:
            return
        for corner in ("tl", "tr", "bl", "br"):
            win = NSWindow.alloc().initWithContentRect_styleMask_backing_defer_(
                NSMakeRect(0, 0, _HANDLE, _HANDLE),
                NSBorderlessWindowMask,
                NSBackingStoreBuffered,
                False,
            )
            win.setLevel_(NSScreenSaverWindowLevel)
            win.setOpaque_(False)
            win.setBackgroundColor_(NSColor.clearColor())
            win.setIgnoresMouseEvents_(False)
            win.setCollectionBehavior_(
                NSWindowCollectionBehaviorCanJoinAllSpaces
                | NSWindowCollectionBehaviorFullScreenAuxiliary
            )
            view = RegionHandleView.alloc().initWithCorner_controller_(corner, self)
            win.setContentView_(view)
            self._handles[corner] = win

    def _position_handles(self) -> None:
        if not self._handles:
            return
        x = self.region.region_x
        y = self.region.region_y
        w = self.region.region_width
        h = self.region.region_height
        # Quartz top-left corners; convert each to a Cocoa point for placement.
        cg_corners = {
            "tl": (x, y),
            "tr": (x + w, y),
            "bl": (x, y + h),
            "br": (x + w, y + h),
        }
        for corner, (gx, gy) in cg_corners.items():
            ccx, ccy = cg_point_to_cocoa(gx, gy)
            win = self._handles[corner]
            win.setFrame_display_(
                NSMakeRect(ccx - _HANDLE / 2, ccy - _HANDLE / 2, _HANDLE, _HANDLE), True
            )
            win.orderFrontRegardless()

def _hex_to_rgb(hex_color: str) -> tuple[float, float, float]:
    hex_color = hex_color.lstrip("#")
    if len(hex_color) != 6:
        return 0.23, 0.51, 0.96
    r = int(hex_color[0:2], 16) / 255.0
    g = int(hex_color[2:4], 16) / 255.0
    b = int(hex_color[4:6], 16) / 255.0
    return r, g, b
