/**
 * @implements [SRS-IN-01] Primitive figures in world space
 *
 * Bounds-first shapes + polyline ink (handwriting dense points).
 * ml-mindmap used AABB shapes + connectors; Infini needs polylines for RM ink.
 */

export type PrimitiveKind = "line" | "rect" | "ellipse" | "path";

export interface Style {
  stroke: string;
  strokeWidth: number; // world units
  fill?: string;
}

export interface BasePrimitive {
  id: string;
  kind: PrimitiveKind;
  style: Style;
  /** World AABB for spatial index / culling. */
  bounds: { minX: number; minY: number; maxX: number; maxY: number };
}

export interface LinePrimitive extends BasePrimitive {
  kind: "line";
  x1: number;
  y1: number;
  x2: number;
  y2: number;
}

export interface RectPrimitive extends BasePrimitive {
  kind: "rect";
  x: number;
  y: number;
  w: number;
  h: number;
}

export interface EllipsePrimitive extends BasePrimitive {
  kind: "ellipse";
  cx: number;
  cy: number;
  rx: number;
  ry: number;
}

export interface PathPrimitive extends BasePrimitive {
  kind: "path";
  /** Dense polyline samples in world space. */
  points: { x: number; y: number }[];
}

export type Primitive =
  | LinePrimitive
  | RectPrimitive
  | EllipsePrimitive
  | PathPrimitive;

export function boundsOfPoints(
  pts: { x: number; y: number }[],
  pad = 0,
): BasePrimitive["bounds"] {
  let minX = Infinity,
    minY = Infinity,
    maxX = -Infinity,
    maxY = -Infinity;
  for (const p of pts) {
    minX = Math.min(minX, p.x);
    minY = Math.min(minY, p.y);
    maxX = Math.max(maxX, p.x);
    maxY = Math.max(maxY, p.y);
  }
  if (!Number.isFinite(minX)) {
    return { minX: 0, minY: 0, maxX: 0, maxY: 0 };
  }
  return {
    minX: minX - pad,
    minY: minY - pad,
    maxX: maxX + pad,
    maxY: maxY + pad,
  };
}

export function makeLine(
  id: string,
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  style: Style,
): LinePrimitive {
  const pad = style.strokeWidth;
  return {
    id,
    kind: "line",
    x1,
    y1,
    x2,
    y2,
    style,
    bounds: {
      minX: Math.min(x1, x2) - pad,
      minY: Math.min(y1, y2) - pad,
      maxX: Math.max(x1, x2) + pad,
      maxY: Math.max(y1, y2) + pad,
    },
  };
}

export function makeRect(
  id: string,
  x: number,
  y: number,
  w: number,
  h: number,
  style: Style,
): RectPrimitive {
  const pad = style.strokeWidth;
  return {
    id,
    kind: "rect",
    x,
    y,
    w,
    h,
    style,
    bounds: {
      minX: x - pad,
      minY: y - pad,
      maxX: x + w + pad,
      maxY: y + h + pad,
    },
  };
}

export function makeEllipse(
  id: string,
  cx: number,
  cy: number,
  rx: number,
  ry: number,
  style: Style,
): EllipsePrimitive {
  const pad = style.strokeWidth;
  return {
    id,
    kind: "ellipse",
    cx,
    cy,
    rx,
    ry,
    style,
    bounds: {
      minX: cx - rx - pad,
      minY: cy - ry - pad,
      maxX: cx + rx + pad,
      maxY: cy + ry + pad,
    },
  };
}

export function makePath(
  id: string,
  points: { x: number; y: number }[],
  style: Style,
): PathPrimitive {
  return {
    id,
    kind: "path",
    points,
    style,
    bounds: boundsOfPoints(points, style.strokeWidth),
  };
}

/** Demo document for canvas.populated (matches design hi-fi figures). */
export function demoPrimitives(): Primitive[] {
  const ink: Style = { stroke: "#1C2430", strokeWidth: 2.5 };
  return [
    makeEllipse("demo-circle", 0, 0, 60, 60, ink),
    makeRect("demo-rect", 80, -40, 160, 100, ink),
    makeLine("demo-line", -200, 40, -20, -20, ink),
    makePath(
      "demo-path",
      [
        { x: -40, y: 100 },
        { x: 0, y: 140 },
        { x: 60, y: 120 },
        { x: 100, y: 160 },
      ],
      ink,
    ),
  ];
}
