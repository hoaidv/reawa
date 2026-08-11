/**
 * @implements [SRS-IN-01] Viewport transform — Infini SRS formula
 *
 * screen = (world + translate) * scale
 * world  = screen / scale - translate
 *
 * Contrast with ml-mindmap: there translate lives in *screen* CSS px
 * (screen = world*s + t_screen). We keep translate in *world* units so
 * the SRS inverse matches hit-testing and epaper drawing-region math.
 */

export interface Vec2 {
  x: number;
  y: number;
}

export interface Viewport {
  /** World-space pan offset (SRS translate). */
  translate: Vec2;
  /** Uniform scale > 0. */
  scale: number;
}

export interface Aabb {
  minX: number;
  minY: number;
  maxX: number;
  maxY: number;
}

export function identityViewport(): Viewport {
  return { translate: { x: 0, y: 0 }, scale: 1 };
}

export function worldToScreen(world: Vec2, vp: Viewport): Vec2 {
  return {
    x: (world.x + vp.translate.x) * vp.scale,
    y: (world.y + vp.translate.y) * vp.scale,
  };
}

export function screenToWorld(screen: Vec2, vp: Viewport): Vec2 {
  return {
    x: screen.x / vp.scale - vp.translate.x,
    y: screen.y / vp.scale - vp.translate.y,
  };
}

/** Axis-aligned world AABB of the visible CSS window. */
export function visibleWorldAabb(
  cssWidth: number,
  cssHeight: number,
  vp: Viewport,
): Aabb {
  const a = screenToWorld({ x: 0, y: 0 }, vp);
  const b = screenToWorld({ x: cssWidth, y: cssHeight }, vp);
  return {
    minX: Math.min(a.x, b.x),
    minY: Math.min(a.y, b.y),
    maxX: Math.max(a.x, b.x),
    maxY: Math.max(a.y, b.y),
  };
}

/**
 * Zoom about a screen focal point, keeping that world point fixed.
 * Learned from ml-mindmap WheelLayer; adapted to world-space translate.
 */
export function zoomAtScreenPoint(
  vp: Viewport,
  screenFocus: Vec2,
  newScale: number,
): Viewport {
  const s = Math.max(0.02, Math.min(64, newScale));
  const world = screenToWorld(screenFocus, vp);
  return {
    scale: s,
    translate: {
      x: screenFocus.x / s - world.x,
      y: screenFocus.y / s - world.y,
    },
  };
}

/** Pan by a screen-pixel delta (content follows finger / trackpad). */
export function panByScreenDelta(vp: Viewport, dx: number, dy: number): Viewport {
  return {
    scale: vp.scale,
    translate: {
      x: vp.translate.x + dx / vp.scale,
      y: vp.translate.y + dy / vp.scale,
    },
  };
}

/**
 * On window resize, keep the world point that was under the *old* center
 * under the *new* center (SRS-UI locked decision).
 */
export function preserveCenterOnResize(
  vp: Viewport,
  oldW: number,
  oldH: number,
  newW: number,
  newH: number,
): Viewport {
  const worldCenter = screenToWorld({ x: oldW / 2, y: oldH / 2 }, vp);
  return {
    scale: vp.scale,
    translate: {
      x: newW / (2 * vp.scale) - worldCenter.x,
      y: newH / (2 * vp.scale) - worldCenter.y,
    },
  };
}

export function aabbIntersects(a: Aabb, b: Aabb): boolean {
  return !(a.maxX < b.minX || a.minX > b.maxX || a.maxY < b.minY || a.minY > b.maxY);
}

export function aabbContains(outer: Aabb, inner: Aabb): boolean {
  return (
    inner.minX >= outer.minX &&
    inner.maxX <= outer.maxX &&
    inner.minY >= outer.minY &&
    inner.maxY <= outer.maxY
  );
}
