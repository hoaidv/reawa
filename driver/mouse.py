from __future__ import annotations

import Quartz

from remarkable import rm2
from remarkable.models.connection import DeviceConfig


class MouseController:
    def __init__(self, config: DeviceConfig) -> None:
        self.config = config

    def current_cursor(self) -> tuple[float, float]:
        loc = Quartz.CGEventGetLocation(Quartz.CGEventCreate(None))
        return loc.x, loc.y

    def desktop_bounds(self) -> tuple[float, float, float, float]:
        max_displays = 16
        err, display_ids, count = Quartz.CGGetActiveDisplayList(max_displays, None, None)
        if err != 0 or not count:
            bounds = Quartz.CGDisplayBounds(Quartz.CGMainDisplayID())
            return (
                bounds.origin.x,
                bounds.origin.y,
                bounds.origin.x + bounds.size.width,
                bounds.origin.y + bounds.size.height,
            )

        min_x = min_y = float("inf")
        max_x = max_y = float("-inf")
        for display_id in display_ids[:count]:
            b = Quartz.CGDisplayBounds(display_id)
            min_x = min(min_x, b.origin.x)
            min_y = min(min_y, b.origin.y)
            max_x = max(max_x, b.origin.x + b.size.width)
            max_y = max(max_y, b.origin.y + b.size.height)
        return min_x, min_y, max_x, max_y

    def clamp(self, x: float, y: float) -> tuple[float, float]:
        min_x, min_y, max_x, max_y = self.desktop_bounds()
        return (
            max(min_x, min(max_x - 1, x)),
            max(min_y, min(max_y - 1, y)),
        )

    def clamp_to_rect(
        self, x: float, y: float, rx: float, ry: float, rw: float, rh: float
    ) -> tuple[float, float]:
        return (
            max(rx, min(rx + rw - 1, x)),
            max(ry, min(ry + rh - 1, y)),
        )

    def display_at_point(self, x: float, y: float) -> int:
        point = Quartz.CGPointMake(x, y)
        err, display_ids, count = Quartz.CGGetDisplaysWithPoint(point, 16, None, None)
        if err == 0 and count:
            return display_ids[0]

        err, display_ids, count = Quartz.CGGetActiveDisplayList(16, None, None)
        if err == 0 and count:
            for display_id in display_ids[:count]:
                b = Quartz.CGDisplayBounds(display_id)
                if (
                    b.origin.x <= x < b.origin.x + b.size.width
                    and b.origin.y <= y < b.origin.y + b.size.height
                ):
                    return display_id
        return Quartz.CGMainDisplayID()

    def display_logical_ppi(self, display_id: int) -> float | None:
        bounds = Quartz.CGDisplayBounds(display_id)
        size_mm = Quartz.CGDisplayScreenSize(display_id)
        if size_mm.width <= 0:
            return None
        return bounds.size.width / (size_mm.width / 25.4)

    def effective_scale(self, cx: float, cy: float) -> float:
        if self.config.scale is not None:
            return self.config.scale
        display_id = self.display_at_point(cx, cy)
        ppi = self.display_logical_ppi(display_id)
        if ppi is None:
            return 1.0
        return ppi / rm2.RM2_DPI

    def map_delta(self, dx: int, dy: int, scale: float) -> tuple[float, float]:
        if self.config.swap_xy:
            dx, dy = dy, dx
        if self.config.invert_x:
            dx = -dx
        if self.config.invert_y:
            dy = -dy
        return dx * scale, dy * scale

    def map_pen_coords(self, pen_x: int, pen_y: int) -> tuple[int, int]:
        x, y = pen_x, pen_y
        if self.config.swap_xy:
            x, y = y, x
        if self.config.invert_x:
            x = rm2.PEN_X_MAX - x
        if self.config.invert_y:
            y = rm2.PEN_Y_MAX - y
        return x, y

    def post_mouse_event(self, event_type: int, x: float, y: float) -> None:
        point = Quartz.CGPointMake(x, y)
        evt = Quartz.CGEventCreateMouseEvent(
            None, event_type, point, Quartz.kCGMouseButtonLeft
        )
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, evt)

    def release_button(self, x: float, y: float) -> None:
        self.post_mouse_event(Quartz.kCGEventLeftMouseUp, x, y)
