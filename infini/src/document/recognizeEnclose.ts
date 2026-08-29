/**
 * Tool-armed enclose recognition → immediate create_smart_group.
 * @implements [SRS-IN-10] enclose recognition (no propose/accept)
 */

import { seedLayoutOffset, smartGroupWorldAabb } from "./anchors";
import { tryDrawIntoMembership } from "./membership";
import type { UndoRing } from "./UndoRing";
import type { VectorDocument } from "./VectorDocument";
import type {
  DocNode,
  InkNode,
  InkSample,
  SmartBounds,
  Style,
} from "./types";
import { DEFAULT_STYLE, IDENTITY_SMART_TRANSFORM } from "./types";

/** ADR-0013 §6 / SRS-IN-10 — shorter side minimum in world units. */
export const MIN_ENCLOSE_WORLD = 48;

export type StrokeIntent = "enclose" | "ink";

export interface EncloseStrokeInput {
  id: string;
  points: ReadonlyArray<{ x: number; y: number }>;
  width?: number;
  /** Absent or `ink` → recognition skipped. */
  intent?: StrokeIntent | string;
  source?: "epaper" | "infini";
}

export type EncloseResult =
  | { kind: "created"; smartGroupId: string }
  | { kind: "ordinary_ink"; reason: string }
  | { kind: "skipped"; reason: string };

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
  if (!Number.isFinite(minX)) {
    return { x: 0, y: 0, width: 0, height: 0 };
  }
  return {
    x: minX,
    y: minY,
    width: Math.max(0, maxX - minX),
    height: Math.max(0, maxY - minY),
  };
}

/** Fraction of samples inside inclusive AABB. */
export function fractionSamplesInside(
  samples: ReadonlyArray<{ x: number; y: number }>,
  bounds: SmartBounds,
): number {
  if (samples.length === 0) return 0;
  const maxX = bounds.x + bounds.width;
  const maxY = bounds.y + bounds.height;
  let inside = 0;
  for (const s of samples) {
    if (
      s.x >= bounds.x &&
      s.x <= maxX &&
      s.y >= bounds.y &&
      s.y <= maxY
    ) {
      inside++;
    }
  }
  return inside / samples.length;
}

function walkInkCandidates(
  nodes: readonly DocNode[],
  parentIsSmartGroup: boolean,
  out: InkNode[],
): void {
  for (const n of nodes) {
    if (n.kind === "ink") {
      if (!parentIsSmartGroup) out.push(n);
      continue;
    }
    if (n.kind === "smart_group") {
      // Children already grouped — skip (SRS-IN-10 guard).
      continue;
    }
    if (n.kind === "frame" || n.kind === "group") {
      walkInkCandidates(n.children, false, out);
    }
  }
}

/**
 * Commit live stroke; if intent enclose and guards pass, create Smart Group.
 * @implements [SRS-IN-10] stroke_end enclose path
 */
export function commitStrokeWithEncloseRecognition(
  tree: VectorDocument,
  undo: UndoRing,
  stroke: EncloseStrokeInput,
): EncloseResult {
  if (stroke.points.length < 2) {
    return { kind: "skipped", reason: "too_few_samples" };
  }

  const intent = stroke.intent ?? "ink";
  if (intent !== "enclose") {
    const r = appendOrdinary(tree, stroke);
    if (!r.applied) {
      return { kind: "skipped", reason: r.reason ?? "append_failed" };
    }
    // SRS-IN-15 — never on enclose; ordinary ink may join a group
    tryDrawIntoMembership(tree, undo, stroke.id);
    return { kind: "ordinary_ink", reason: "not_enclose_intent" };
  }

  const worldBounds = samplesAabb(stroke.points);
  const shorter = Math.min(worldBounds.width, worldBounds.height);
  if (shorter < MIN_ENCLOSE_WORLD) {
    appendOrdinary(tree, stroke);
    tryDrawIntoMembership(tree, undo, stroke.id);
    return { kind: "ordinary_ink", reason: "too_small" };
  }

  const candidates: InkNode[] = [];
  walkInkCandidates(tree.rootChildren, false, candidates);
  const capturable = candidates.filter(
    (ink) => fractionSamplesInside(ink.samples, worldBounds) >= 0.8,
  );
  if (capturable.length === 0) {
    const insideExisting = tree.rootChildren.some((n) => {
      if (n.kind !== "smart_group") return false;
      const w = smartGroupWorldAabb(n);
      return (
        fractionSamplesInside(stroke.points, {
          x: w.minX,
          y: w.minY,
          width: w.maxX - w.minX,
          height: w.maxY - w.minY,
        }) >= 0.8
      );
    });
    if (insideExisting) {
      appendOrdinary(tree, stroke);
      tryDrawIntoMembership(tree, undo, stroke.id);
      return { kind: "ordinary_ink", reason: "no_content" };
    }
  }

  // Local-space Smart Group: bounds origin at (0,0), transform carries world origin.
  // @implements [SRS-IN-10] enclose → create_smart_group geometry
  const bounds: SmartBounds = {
    x: 0,
    y: 0,
    width: worldBounds.width,
    height: worldBounds.height,
  };
  const transform = {
    ...IDENTITY_SMART_TRANSFORM,
    x: worldBounds.x,
    y: worldBounds.y,
  };
  const toLocal = (p: { x: number; y: number }) => ({
    x: p.x - worldBounds.x,
    y: p.y - worldBounds.y,
  });

  const style: Style = {
    ...DEFAULT_STYLE,
    strokeWidth: stroke.width ?? DEFAULT_STYLE.strokeWidth,
  };
  const boundarySamples: InkSample[] = stroke.points.map((p, i) => ({
    ...toLocal(p),
    t: i,
  }));
  const boundary: InkNode = {
    id: stroke.id,
    kind: "ink",
    role: "boundary",
    samples: boundarySamples,
    style,
  };

  const contentChildren: InkNode[] = capturable.map((ink) => {
    const cloned = JSON.parse(JSON.stringify(ink)) as InkNode;
    cloned.role = "content";
    cloned.samples = cloned.samples.map((s, i) => ({
      ...s,
      x: s.x - worldBounds.x,
      y: s.y - worldBounds.y,
      t: s.t ?? i,
    }));
    cloned.layoutOffset = seedLayoutOffset(cloned.samples, bounds);
    return cloned;
  });

  const smartGroupId = `sg_enclose_${stroke.id}`;
  const result = undo.applyWithUndo(tree, {
    opId: `create_smart_group:${smartGroupId}`,
    type: "create_smart_group",
    source: stroke.source ?? "infini",
    payload: {
      id: smartGroupId,
      bounds,
      transform,
      inkScaleMode: "fixedInk",
      captureIds: capturable.map((c) => c.id),
      children: [boundary, ...contentChildren],
      boundaryPolyline: (() => {
        const poly = boundarySamples.map((s) => ({ ...s }));
        const a = poly[0];
        const b = poly[poly.length - 1];
        if (poly.length >= 2 && Math.hypot(a.x - b.x, a.y - b.y) > 1e-6) poly.push({ ...a });
        return poly;
      })(),
    },
  });

  if (!result.applied) {
    appendOrdinary(tree, stroke);
    return {
      kind: "ordinary_ink",
      reason: result.reason ?? "create_failed",
    };
  }
  return { kind: "created", smartGroupId };
}

function appendOrdinary(
  tree: VectorDocument,
  stroke: EncloseStrokeInput,
): { applied: boolean; reason?: string } {
  const samples: InkSample[] = stroke.points.map((p, i) => ({
    x: p.x,
    y: p.y,
    t: i,
  }));
  return tree.applyOp({
    opId: `append_ink:${stroke.id}`,
    type: "append_ink",
    source: stroke.source ?? "epaper",
    payload: {
      id: stroke.id,
      samples,
      style: {
        ...DEFAULT_STYLE,
        strokeWidth: stroke.width ?? DEFAULT_STYLE.strokeWidth,
      },
    },
  });
}
