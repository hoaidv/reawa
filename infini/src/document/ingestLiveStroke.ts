/**
 * Commit a finished live stroke into VectorDocument (tree SoT).
 * @implements [SRS-IN-04] tree-backed live ink ingestion
 */

import type { VectorDocument } from "./VectorDocument";
import type { InkSample, Style } from "./types";
import { DEFAULT_STYLE } from "./types";

export interface LiveStrokeCommit {
  id: string;
  points: ReadonlyArray<{ x: number; y: number }>;
  width?: number;
  parentId?: string;
  source?: "epaper" | "infini";
}

/**
 * Apply append_ink for a committed stroke. Idempotent on stroke id as opId.
 * @implements [SRS-IN-04] append_ink on live stroke path
 */
export function commitLiveStrokeToTree(
  tree: VectorDocument,
  stroke: LiveStrokeCommit,
): { applied: boolean; reason?: string } {
  if (stroke.points.length < 2) {
    return { applied: false, reason: "too_few_samples" };
  }
  const samples: InkSample[] = stroke.points.map((p, i) => ({
    x: p.x,
    y: p.y,
    t: i,
  }));
  const style: Style = {
    ...DEFAULT_STYLE,
    strokeWidth: stroke.width ?? DEFAULT_STYLE.strokeWidth,
  };
  return tree.applyOp({
    opId: `append_ink:${stroke.id}`,
    type: "append_ink",
    source: stroke.source ?? "epaper",
    payload: {
      id: stroke.id,
      parentId: stroke.parentId,
      samples,
      style,
    },
  });
}
