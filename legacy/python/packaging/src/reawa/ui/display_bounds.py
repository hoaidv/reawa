from __future__ import annotations

from AppKit import NSScreen

# Region + window coordinates are stored in Quartz "global display" space
# (top-left origin, y increases downward). That space is what CGEvent mouse
# events, CGEventGetLocation, CGWindowList bounds, and the Accessibility
# position/size attributes all use. We only convert to Cocoa (bottom-left)
# coordinates when drawing AppKit windows/views.


def primary_height() -> float:
    """Height of the main display, the pivot for Quartz<->Cocoa Y flips."""
    screens = NSScreen.screens()
    if not screens:
        return NSScreen.mainScreen().frame().size.height
    return screens[0].frame().size.height


def cg_rect_to_cocoa(x: float, y: float, w: float, h: float) -> tuple[float, float, float, float]:
    """Quartz top-left rect -> Cocoa bottom-left rect (both global)."""
    return x, primary_height() - y - h, w, h


def cg_point_to_cocoa(x: float, y: float) -> tuple[float, float]:
    return x, primary_height() - y


def desktop_bounds_cocoa() -> tuple[float, float, float, float]:
    """Union of all display frames in Cocoa coordinates (for sizing overlays)."""
    min_x = min_y = float("inf")
    max_x = max_y = float("-inf")
    for screen in NSScreen.screens():
        frame = screen.frame()
        min_x = min(min_x, frame.origin.x)
        min_y = min(min_y, frame.origin.y)
        max_x = max(max_x, frame.origin.x + frame.size.width)
        max_y = max(max_y, frame.origin.y + frame.size.height)
    if min_x == float("inf"):
        frame = NSScreen.mainScreen().frame()
        return (
            frame.origin.x,
            frame.origin.y,
            frame.origin.x + frame.size.width,
            frame.origin.y + frame.size.height,
        )
    return min_x, min_y, max_x, max_y


# Backwards-compatible alias used by overlay/picker for window framing.
def desktop_bounds() -> tuple[float, float, float, float]:
    return desktop_bounds_cocoa()
