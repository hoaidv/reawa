/**
 * Snapshot-based undo ring for VectorDocument structural ops.
 * @implements [SRS-IN-12] undo history ring depth 20
 */

import type { VectorDocument } from "./VectorDocument";
import type { DocNode, DocOp } from "./types";

export const UNDO_RING_DEPTH = 20;

const STRUCTURAL_TYPES = new Set([
  "create_frame",
  "create_group",
  "create_text",
  "create_primitive",
  "append_ink",
  "create_connector",
  "create_smart_group",
  "set_smart_transform",
  "set_ink_scale_mode",
  "join_smart_group",
  "reparent",
  "remove_node",
  "translate_node",
]);

export function isStructuralOp(type: string): boolean {
  return STRUCTURAL_TYPES.has(type);
}

export class UndoRing {
  private readonly ring: string[] = [];

  get depth(): number {
    return this.ring.length;
  }

  /**
   * Push a pre-op snapshot. Drops oldest when over depth.
   * @implements [SRS-IN-12] ring overflow drops oldest
   */
  pushSnapshot(snapshot: string): void {
    this.ring.push(snapshot);
    while (this.ring.length > UNDO_RING_DEPTH) {
      this.ring.shift();
    }
  }

  /**
   * Apply op after pushing current tree snapshot when structural.
   * @implements [SRS-IN-12] push before every structural op
   */
  applyWithUndo(
    tree: VectorDocument,
    op: DocOp,
  ): { applied: boolean; reason?: string; pushed: boolean } {
    const structural = isStructuralOp(op.type);
    if (structural) {
      this.pushSnapshot(tree.snapshotString());
    }
    const result = tree.applyOp(op);
    if (structural && !result.applied) {
      // Roll back the unused snapshot push for failed/duplicate ops
      this.ring.pop();
      return { ...result, pushed: false };
    }
    return { ...result, pushed: structural && result.applied };
  }

  /**
   * Restore previous snapshot wholesale.
   * @implements [SRS-IN-12] undo restores pre-op snapshot
   */
  undo(tree: VectorDocument): { restored: boolean } {
    const snap = this.ring.pop();
    if (!snap) return { restored: false };
    const parsed = JSON.parse(snap) as {
      rootChildren: DocNode[];
      status?: string;
    };
    tree.replaceTree(parsed.rootChildren);
    if (parsed.status === "none" || parsed.status === "open" || parsed.status === "dirty" || parsed.status === "error") {
      tree.status = parsed.status;
    }
    return { restored: true };
  }
}
