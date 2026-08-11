/**
 * Anchor → world point from live node geometry.
 * @implements [SRS-IN-04] connector boundary anchor resolve
 */

import type {
  Aabb,
  Anchor,
  DocNode,
  EllipseGeom,
  LineGeom,
  PortId,
  PrimitiveNode,
  RectPort,
  SmartGroupNode,
  Vec2,
} from "./types";

function aabbFromRect(x: number, y: number, w: number, h: number): Aabb {
  return { minX: x, minY: y, maxX: x + w, maxY: y + h };
}

function pointOnAabbEdge(box: Aabb, edge: RectPort, t: number): Vec2 {
  const u = Math.min(1, Math.max(0, t));
  const { minX, minY, maxX, maxY } = box;
  switch (edge) {
    case "north":
      return { x: minX + (maxX - minX) * u, y: minY };
    case "south":
      return { x: minX + (maxX - minX) * u, y: maxY };
    case "west":
      return { x: minX, y: minY + (maxY - minY) * u };
    case "east":
      return { x: maxX, y: minY + (maxY - minY) * u };
  }
}

function portOnAabb(box: Aabb, port: PortId): Vec2 | null {
  switch (port) {
    case "north":
      return pointOnAabbEdge(box, "north", 0.5);
    case "south":
      return pointOnAabbEdge(box, "south", 0.5);
    case "east":
      return pointOnAabbEdge(box, "east", 0.5);
    case "west":
      return pointOnAabbEdge(box, "west", 0.5);
    default:
      return null;
  }
}

function resolveOnAabb(box: Aabb, anchor: Anchor): Vec2 | null {
  if (anchor.port) {
    return portOnAabb(box, anchor.port);
  }
  if (anchor.boundary && "edge" in anchor.boundary) {
    return pointOnAabbEdge(box, anchor.boundary.edge, anchor.boundary.t);
  }
  return null;
}

function resolveOnEllipse(g: EllipseGeom, anchor: Anchor): Vec2 | null {
  const { cx, cy, rx, ry } = g;
  if (anchor.port) {
    switch (anchor.port) {
      case "top":
        return { x: cx, y: cy - ry };
      case "bottom":
        return { x: cx, y: cy + ry };
      case "left":
        return { x: cx - rx, y: cy };
      case "right":
        return { x: cx + rx, y: cy };
      default:
        return null;
    }
  }
  if (anchor.boundary && "angle" in anchor.boundary) {
    const a = anchor.boundary.angle;
    return { x: cx + rx * Math.cos(a), y: cy + ry * Math.sin(a) };
  }
  if (anchor.boundary && "t" in anchor.boundary && !("edge" in anchor.boundary)) {
    const a = anchor.boundary.t * Math.PI * 2;
    return { x: cx + rx * Math.cos(a), y: cy + ry * Math.sin(a) };
  }
  return null;
}

function resolveOnLine(g: LineGeom, anchor: Anchor): Vec2 | null {
  const { x1, y1, x2, y2 } = g;
  const along = (t: number): Vec2 => ({
    x: x1 + (x2 - x1) * t,
    y: y1 + (y2 - y1) * t,
  });
  if (anchor.port === "start") return { x: x1, y: y1 };
  if (anchor.port === "end") return { x: x2, y: y2 };
  if (anchor.port === "mid") return along(0.5);
  if (anchor.boundary && "t" in anchor.boundary && !("edge" in anchor.boundary) && !("angle" in anchor.boundary)) {
    return along(anchor.boundary.t);
  }
  return null;
}

/** World AABB of SmartGroup geometric bounds after transform (v0: translate + scale; rotation deferred for ports). */
export function smartGroupWorldAabb(sg: SmartGroupNode): Aabb {
  const { bounds, transform: t } = sg;
  const x = t.x + bounds.x * t.scaleX;
  const y = t.y + bounds.y * t.scaleY;
  const w = bounds.width * t.scaleX;
  const h = bounds.height * t.scaleY;
  return aabbFromRect(x, y, w, h);
}

export function nodeWorldAabb(node: DocNode): Aabb | null {
  switch (node.kind) {
    case "primitive":
      return primitiveAabb(node);
    case "text":
      return { ...node.box };
    case "frame":
      return { ...node.bounds };
    case "smart_group":
      return smartGroupWorldAabb(node);
    case "ink": {
      let minX = Infinity,
        minY = Infinity,
        maxX = -Infinity,
        maxY = -Infinity;
      for (const s of node.samples) {
        minX = Math.min(minX, s.x);
        minY = Math.min(minY, s.y);
        maxX = Math.max(maxX, s.x);
        maxY = Math.max(maxY, s.y);
      }
      if (!Number.isFinite(minX)) return { minX: 0, minY: 0, maxX: 0, maxY: 0 };
      return { minX, minY, maxX, maxY };
    }
    case "group": {
      let minX = Infinity,
        minY = Infinity,
        maxX = -Infinity,
        maxY = -Infinity;
      for (const c of node.children) {
        const b = nodeWorldAabb(c);
        if (!b) continue;
        minX = Math.min(minX, b.minX);
        minY = Math.min(minY, b.minY);
        maxX = Math.max(maxX, b.maxX);
        maxY = Math.max(maxY, b.maxY);
      }
      if (!Number.isFinite(minX)) return { minX: 0, minY: 0, maxX: 0, maxY: 0 };
      return { minX, minY, maxX, maxY };
    }
    case "connector":
      return null;
  }
}

function primitiveAabb(node: PrimitiveNode): Aabb {
  const g = node.geom;
  const pad = node.style.strokeWidth;
  switch (g.kind) {
    case "rect":
      return {
        minX: g.x - pad,
        minY: g.y - pad,
        maxX: g.x + g.w + pad,
        maxY: g.y + g.h + pad,
      };
    case "ellipse":
      return {
        minX: g.cx - g.rx - pad,
        minY: g.cy - g.ry - pad,
        maxX: g.cx + g.rx + pad,
        maxY: g.cy + g.ry + pad,
      };
    case "line":
      return {
        minX: Math.min(g.x1, g.x2) - pad,
        minY: Math.min(g.y1, g.y2) - pad,
        maxX: Math.max(g.x1, g.x2) + pad,
        maxY: Math.max(g.y1, g.y2) + pad,
      };
  }
}

/**
 * Resolve Anchor → world Vec2 from the live node map.
 * @implements [SRS-IN-04] Anchor → world from live geom
 */
export function resolveAnchor(
  anchor: Anchor,
  byId: Map<string, DocNode>,
): Vec2 | null {
  const node = byId.get(anchor.nodeId);
  if (!node) return null;

  switch (node.kind) {
    case "primitive": {
      if (node.geom.kind === "ellipse") return resolveOnEllipse(node.geom, anchor);
      if (node.geom.kind === "line") return resolveOnLine(node.geom, anchor);
      if (node.geom.kind === "rect") {
        const box = aabbFromRect(node.geom.x, node.geom.y, node.geom.w, node.geom.h);
        return resolveOnAabb(box, anchor);
      }
      return null;
    }
    case "text":
    case "frame":
    case "group": {
      const box = nodeWorldAabb(node);
      return box ? resolveOnAabb(box, anchor) : null;
    }
    case "smart_group":
      return resolveOnAabb(smartGroupWorldAabb(node), anchor);
    default:
      return null;
  }
}

/** Local ink sample → world under SmartGroup transform (v0: translate + uniform-ish scale; rotation for content). */
export function smartLocalToWorld(
  local: Vec2,
  sg: SmartGroupNode,
  role: "content" | "boundary",
): Vec2 {
  const t = sg.transform;
  let x = local.x;
  let y = local.y;
  if (role === "boundary" || sg.inkScaleMode === "withBounds") {
    x *= t.scaleX;
    y *= t.scaleY;
  }
  if (t.rotation !== 0) {
    const c = Math.cos(t.rotation);
    const s = Math.sin(t.rotation);
    const rx = x * c - y * s;
    const ry = x * s + y * c;
    x = rx;
    y = ry;
  }
  return { x: x + t.x, y: y + t.y };
}
