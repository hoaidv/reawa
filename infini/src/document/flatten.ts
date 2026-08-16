/**
 * flattenDrawables — tree → leaves for spatial cull / WorldLayer.
 * @implements [SRS-IN-04] flattenDrawables projection
 */

import { inkSamplesCentroid, nodeWorldAabb, smartLocalToWorld } from "./anchors";
import type {
  Aabb,
  ConnectorDrawable,
  DocNode,
  Drawable,
  InkDrawable,
  InkNode,
  SmartGroupNode,
  Vec2,
} from "./types";

/** Minimal surface used by flatten (avoids circular import with VectorDocument). */
export interface FlattenSource {
  rootChildren: readonly DocNode[];
  indexById(): Map<string, DocNode>;
}

function unionAabb(a: Aabb, b: Aabb): Aabb {
  return {
    minX: Math.min(a.minX, b.minX),
    minY: Math.min(a.minY, b.minY),
    maxX: Math.max(a.maxX, b.maxX),
    maxY: Math.max(a.maxY, b.maxY),
  };
}

function inkDrawable(node: InkNode, samples = node.samples): InkDrawable {
  const bounds = nodeWorldAabb({ ...node, samples }) ?? {
    minX: 0,
    minY: 0,
    maxX: 0,
    maxY: 0,
  };
  return {
    id: node.id,
    kind: "ink",
    samples,
    style: node.style,
    bounds,
  };
}

function walk(nodes: readonly DocNode[], out: Drawable[]): void {
  for (const node of nodes) {
    switch (node.kind) {
      case "ink":
        out.push(inkDrawable(node));
        break;
      case "text":
        out.push({
          id: node.id,
          kind: "text",
          box: node.box,
          runs: node.runs,
          style: node.style,
          bounds: { ...node.box },
        });
        break;
      case "primitive": {
        const bounds = nodeWorldAabb(node)!;
        out.push({
          id: node.id,
          kind: "primitive",
          geom: node.geom,
          style: node.style,
          bounds,
        });
        break;
      }
      case "frame":
      case "group":
        walk(node.children, out);
        break;
      case "smart_group":
        out.push(...flattenSmartGroup(node));
        break;
      case "connector": {
        const path: Vec2[] =
          node.warpedSamples && node.warpedSamples.length >= 2
            ? node.warpedSamples
            : node.path && node.path.length >= 2
              ? node.path
              : [];
        let bounds: Aabb = { minX: 0, minY: 0, maxX: 0, maxY: 0 };
        if (path.length >= 1) {
          bounds = {
            minX: path[0].x,
            minY: path[0].y,
            maxX: path[0].x,
            maxY: path[0].y,
          };
          for (const p of path) {
            bounds.minX = Math.min(bounds.minX, p.x);
            bounds.minY = Math.min(bounds.minY, p.y);
            bounds.maxX = Math.max(bounds.maxX, p.x);
            bounds.maxY = Math.max(bounds.maxY, p.y);
          }
        }
        const fromPt = path[0] ?? { x: 0, y: 0 };
        const toPt = path.length > 0 ? path[path.length - 1] : { x: 0, y: 0 };
        const d: ConnectorDrawable = {
          id: node.id,
          kind: "connector",
          from: fromPt,
          to: toPt,
          path,
          bounds,
          invalid: node.invalid === true,
        };
        out.push(d);
        break;
      }
    }
  }
}

function flattenSmartGroup(sg: SmartGroupNode): InkDrawable[] {
  return sg.children.map((ink) => {
    const role = ink.role ?? "content";
    const centroid =
      role === "content" && sg.inkScaleMode === "fixedInk"
        ? inkSamplesCentroid(ink.samples)
        : undefined;
    const samples = ink.samples.map((s) => {
      const w = smartLocalToWorld(
        { x: s.x, y: s.y },
        sg,
        role,
        ink.layoutOffset,
        centroid,
      );
      return { ...s, x: w.x, y: w.y };
    });
    return inkDrawable({ ...ink, samples });
  });
}

/**
 * @implements [SRS-IN-04] flattenDrawables for WorldLayer
 */
export function flattenDrawables(doc: FlattenSource): Drawable[] {
  const out: Drawable[] = [];
  walk(doc.rootChildren, out);
  return out;
}

/** Re-export helper used by tests / paint bridge. */
export function drawableBoundsUnion(drawables: Drawable[]): Aabb | null {
  if (drawables.length === 0) return null;
  return drawables.reduce((acc, d) => unionAabb(acc, d.bounds), drawables[0].bounds);
}
