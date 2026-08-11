/**
 * Selection create — surround stroke required.
 * @implements [SRS-IN-16] selection create surround
 */

import { seedLayoutOffset } from "./anchors";
import type { UndoRing } from "./UndoRing";
import type { VectorDocument } from "./VectorDocument";
import type { InkNode, SmartBounds } from "./types";
import { DEFAULT_STYLE, IDENTITY_SMART_TRANSFORM } from "./types";

export type SelectionCreateResult =
  | { kind: "created"; smartGroupId: string; boundaryId: string }
  | { kind: "refused"; reason: string };

function samplesAabb(samples: ReadonlyArray<{ x: number; y: number }>): SmartBounds {
  let minX = Infinity,
    minY = Infinity,
    maxX = -Infinity,
    maxY = -Infinity;
  for (const s of samples) {
    minX = Math.min(minX, s.x);
    minY = Math.min(minY, s.y);
    maxX = Math.max(maxX, s.x);
    maxY = Math.max(maxY, s.y);
  }
  return {
    x: minX,
    y: minY,
    width: Math.max(0, maxX - minX),
    height: Math.max(0, maxY - minY),
  };
}

/**
 * Even-odd point-in-polygon. `poly` must be closed (first≈last optional).
 * @implements [SRS-IN-16] even-odd PIP
 */
export function pointInPolygonEvenOdd(
  x: number,
  y: number,
  poly: ReadonlyArray<{ x: number; y: number }>,
): boolean {
  if (poly.length < 3) return false;
  let inside = false;
  for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
    const xi = poly[i].x;
    const yi = poly[i].y;
    const xj = poly[j].x;
    const yj = poly[j].y;
    const intersect =
      yi > y !== yj > y && x < ((xj - xi) * (y - yi)) / (yj - yi + 0.0) + xi;
    if (intersect) inside = !inside;
  }
  return inside;
}

/** Artificial close for open polylines — test path only; samples unchanged. */
export function closedPathForTest(
  samples: ReadonlyArray<{ x: number; y: number }>,
): { x: number; y: number }[] {
  if (samples.length === 0) return [];
  const path = samples.map((s) => ({ x: s.x, y: s.y }));
  const first = path[0];
  const last = path[path.length - 1];
  if (first.x !== last.x || first.y !== last.y) {
    path.push({ x: first.x, y: first.y });
  }
  return path;
}

export function fractionInsidePolygon(
  samples: ReadonlyArray<{ x: number; y: number }>,
  poly: ReadonlyArray<{ x: number; y: number }>,
): number {
  if (samples.length === 0) return 0;
  let n = 0;
  for (const s of samples) {
    if (pointInPolygonEvenOdd(s.x, s.y, poly)) n++;
  }
  return n / samples.length;
}

/**
 * True when S surrounds ≥80% of every other selected ink.
 */
export function qualifiesAsSurround(
  candidate: InkNode,
  others: readonly InkNode[],
): boolean {
  const poly = closedPathForTest(candidate.samples);
  if (poly.length < 4) return false;
  for (const o of others) {
    if (fractionInsidePolygon(o.samples, poly) < 0.8) return false;
  }
  return true;
}

/**
 * Create Smart Group from selected ink ids, or refuse.
 * @implements [SRS-IN-16] selection create
 */
export function createSmartGroupFromSelection(
  tree: VectorDocument,
  undo: UndoRing,
  selectedInkIds: readonly string[],
): SelectionCreateResult {
  if (selectedInkIds.length < 2) {
    return { kind: "refused", reason: "need_at_least_two" };
  }

  const inks: InkNode[] = [];
  for (const id of selectedInkIds) {
    const n = tree.indexById().get(id);
    if (!n || n.kind !== "ink") {
      return { kind: "refused", reason: `missing_ink:${id}` };
    }
    // Must be free (not already in a Smart Group)
    inks.push(n);
  }

  // Preserve selection order for "later sibling" — resolve by tree paint order.
  const paintOrder = listFreeInkPaintOrder(tree);
  const ordered = paintOrder.filter((id) => selectedInkIds.includes(id));
  const orderedInks = ordered
    .map((id) => inks.find((i) => i.id === id)!)
    .filter(Boolean);

  const qualifiers: InkNode[] = [];
  for (const cand of orderedInks) {
    const others = orderedInks.filter((i) => i.id !== cand.id);
    if (qualifiesAsSurround(cand, others)) qualifiers.push(cand);
  }
  if (qualifiers.length === 0) {
    return { kind: "refused", reason: "no_surround" };
  }
  const winner = qualifiers[qualifiers.length - 1];
  const bounds = samplesAabb(winner.samples);
  const content = orderedInks.filter((i) => i.id !== winner.id);

  const boundary: InkNode = {
    ...(JSON.parse(JSON.stringify(winner)) as InkNode),
    role: "boundary",
  };
  delete boundary.layoutOffset;

  const contentNodes: InkNode[] = content.map((ink) => {
    const c = JSON.parse(JSON.stringify(ink)) as InkNode;
    c.role = "content";
    c.layoutOffset = seedLayoutOffset(c.samples, bounds);
    return c;
  });

  const smartGroupId = `sg_sel_${winner.id}`;
  const result = undo.applyWithUndo(tree, {
    opId: `create_smart_group:${smartGroupId}`,
    type: "create_smart_group",
    payload: {
      id: smartGroupId,
      bounds,
      transform: { ...IDENTITY_SMART_TRANSFORM },
      inkScaleMode: "withBounds",
      captureIds: orderedInks.map((i) => i.id),
      children: [boundary, ...contentNodes],
    },
  });

  if (!result.applied) {
    return { kind: "refused", reason: result.reason ?? "create_failed" };
  }
  return { kind: "created", smartGroupId, boundaryId: winner.id };
}

function listFreeInkPaintOrder(tree: VectorDocument): string[] {
  const out: string[] = [];
  const walk = (nodes: readonly import("./types").DocNode[]) => {
    for (const n of nodes) {
      if (n.kind === "ink") out.push(n.id);
      if (n.kind === "frame" || n.kind === "group") walk(n.children);
      // skip smart_group children — not free
    }
  };
  walk(tree.rootChildren);
  return out;
}

/** @deprecated style import keep for tests */
export const _defaultStyle = DEFAULT_STYLE;
