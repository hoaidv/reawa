from __future__ import annotations

import Quartz

from typing import Callable

from . import rm2
from .mouse import MouseController
from ..models.connection import AbsoluteConfig


class AbsoluteDriver:
    def __init__(
        self,
        mouse: MouseController,
        region: AbsoluteConfig,
        on_region_change: Callable | None = None,
    ) -> None:
        self.mouse = mouse
        self.region = region
        self.on_region_change = on_region_change
        self.was_touching = False
        self.button_down = False

    def _pen_to_screen(self, pen_x: int, pen_y: int) -> tuple[float, float]:
        mx, my = self.mouse.map_pen_coords(pen_x, pen_y)
        rx = self.region.region_x
        ry = self.region.region_y
        rw = self.region.region_width
        rh = self.region.region_height
        sx = rx + (mx / rm2.PEN_X_MAX) * rw
        sy = ry + (my / rm2.PEN_Y_MAX) * rh
        return self.mouse.clamp_to_rect(sx, sy, rx, ry, rw, rh)

    def handle_frame(self, frame: rm2.PenFrame) -> None:
        if not frame.in_proximity:
            if self.button_down:
                cx, cy = self.mouse.current_cursor()
                self.mouse.release_button(cx, cy)
                self.button_down = False
            self.was_touching = False
            return

        cx, cy = self._pen_to_screen(frame.x, frame.y)

        if frame.touching and not self.was_touching:
            self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseDown, cx, cy)
            self.button_down = True
        elif not frame.touching and self.was_touching:
            self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseUp, cx, cy)
            self.button_down = False
        elif frame.touching:
            self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseDragged, cx, cy)
        else:
            self.mouse.post_mouse_event(Quartz.kCGEventMouseMoved, cx, cy)

        self.was_touching = frame.touching

    def update_region(self, region: AbsoluteConfig) -> None:
        self.region = region

    def cleanup(self) -> None:
        if self.button_down:
            cx, cy = self.mouse.current_cursor()
            self.mouse.release_button(cx, cy)
            self.button_down = False
