from __future__ import annotations

import Quartz

from remarkable import rm2
from remarkable.driver.mouse import MouseController


class RelativeDriver:
    def __init__(self, mouse: MouseController) -> None:
        self.mouse = mouse
        self.prev_x: int | None = None
        self.prev_y: int | None = None
        self.was_touching = False
        self.button_down = False

    def handle_frame(self, frame: rm2.PenFrame) -> None:
        cx, cy = self.mouse.current_cursor()

        if not frame.in_proximity:
            if self.button_down:
                self.mouse.release_button(cx, cy)
                self.button_down = False
            self.was_touching = False
            self.prev_x = None
            self.prev_y = None
            return

        if self.prev_x is None or self.prev_y is None:
            self.prev_x = frame.x
            self.prev_y = frame.y
            self.was_touching = frame.touching
            if frame.touching and not self.button_down:
                self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseDown, cx, cy)
                self.button_down = True
            return

        dx = frame.x - self.prev_x
        dy = frame.y - self.prev_y
        scale = self.mouse.effective_scale(cx, cy)
        sdx, sdy = self.mouse.map_delta(dx, dy, scale)

        if sdx != 0.0 or sdy != 0.0:
            cx, cy = self.mouse.clamp(cx + sdx, cy + sdy)

        if frame.touching and not self.was_touching:
            self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseDown, cx, cy)
            self.button_down = True
        elif not frame.touching and self.was_touching:
            self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseUp, cx, cy)
            self.button_down = False

        if sdx != 0.0 or sdy != 0.0:
            if frame.touching:
                self.mouse.post_mouse_event(Quartz.kCGEventLeftMouseDragged, cx, cy)
            else:
                self.mouse.post_mouse_event(Quartz.kCGEventMouseMoved, cx, cy)

        self.prev_x = frame.x
        self.prev_y = frame.y
        self.was_touching = frame.touching

    def cleanup(self) -> None:
        if self.button_down:
            cx, cy = self.mouse.current_cursor()
            self.mouse.release_button(cx, cy)
            self.button_down = False
