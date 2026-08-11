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

export type TabletOrientation = "portrait" | "landscape";

/** CSS pixel rect for the tablet drawing frame (marker geometry). */
export interface CssRect {
  x: number;
  y: number;
  w: number;
  h: number;
}

/** RM2 panel pixels (native portrait). @implements [SRS-IN-07] */
export const TABLET_PANEL_W = 1404;
export const TABLET_PANEL_H = 1872;

/** Aspect width/height for the sync frame. */
export function tabletAspect(orientation: TabletOrientation = "portrait"): number {
  return orientation === "landscape"
    ? TABLET_PANEL_H / TABLET_PANEL_W
    : TABLET_PANEL_W / TABLET_PANEL_H;
}

/**
 * Largest centered CSS frame matching tablet aspect — maximize width or height
 * to the host viewport; center the other axis (no margin letterbox pad).
 * @implements [SRS-IN-07] tablet drawing frame (CSS)
 */
export function tabletDrawingFrameCss(
  cssW: number,
  cssH: number,
  orientation: TabletOrientation = "portrait",
): CssRect {
  const aspect = tabletAspect(orientation);
  let w = cssW;
  let h = w / aspect;
  if (h > cssH) {
    h = cssH;
    w = h * aspect;
  }
  return { x: (cssW - w) / 2, y: (cssH - h) / 2, w, h };
}

/**
 * Map panel-framebuffer coords (after digitizer→panel map) → sync-frame UV.
 * Portrait: 1:1 with panel. Landscape: 90° so wide Infini frame matches rotated use.
 * @implements [SRS-IN-07] orientation
 */
export function panelToFrameUv(
  localX: number,
  localY: number,
  panelW: number,
  panelH: number,
  orientation: TabletOrientation,
): { u: number; v: number } {
  if (orientation === "landscape") {
    const logX = localY;
    const logY = panelW - localX;
    return { u: logX / panelH, v: logY / panelW };
  }
  return { u: localX / panelW, v: localY / panelH };
}

/** Inverse of panelToFrameUv for Epaper vector paint (world UV → panel px). */
export function frameUvToPanel(
  u: number,
  v: number,
  panelW: number,
  panelH: number,
  orientation: TabletOrientation,
): { x: number; y: number } {
  if (orientation === "landscape") {
    const logX = u * panelH;
    const logY = v * panelW;
    return { x: panelW - logY, y: logX };
  }
  return { x: u * panelW, y: v * panelH };
}

/**
 * World AABB of a CSS tablet frame under the current viewport.
 * @implements [SRS-IN-07] drawingRegion = screenToWorld(frame)
 */
export function frameWorldAabb(frame: CssRect, vp: Viewport): Aabb {
  const a = screenToWorld({ x: frame.x, y: frame.y }, vp);
  const b = screenToWorld({ x: frame.x + frame.w, y: frame.y + frame.h }, vp);
  return {
    minX: Math.min(a.x, b.x),
    minY: Math.min(a.y, b.y),
    maxX: Math.max(a.x, b.x),
    maxY: Math.max(a.y, b.y),
  };
}

/**
 * CSS line width for world stroke under Infini viewport (CTM path uses world×scale).
 * @implements [SRS-IN-08] / [ADR-0012] world width × scale
 */
export function strokeCssWidthFromWorld(strokeWidthWorld: number, scale: number): number {
  return strokeWidthWorld * scale;
}

/**
 * Convert panel-pixel brush width into world units for a drawing region.
 * @implements [ADR-0012] panel px → world
 */
export function strokeWorldWidthFromPanel(
  strokeWidthPanelPx: number,
  drawingRegion: Aabb,
  panelW: number,
): number {
  const worldW = drawingRegion.maxX - drawingRegion.minX;
  if (panelW <= 0 || worldW <= 0) return strokeWidthPanelPx;
  return strokeWidthPanelPx * (worldW / panelW);
}
