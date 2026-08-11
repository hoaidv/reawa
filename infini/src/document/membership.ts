/**
 * Draw-into membership — ordinary ink joins an existing Smart Group.
 * @implements [SRS-IN-15] draw-into membership
 */

import { seedLayoutOffset, smartGroupWorldAabb } from "./anchors";
import { fractionSamplesInside } from "./recognizeEnclose";
import type { UndoRing } from "./UndoRing";
import type { VectorDocument } from "./VectorDocument";
import type { DocNode, InkNode, SmartBounds, SmartGroupNode } from "./types";

export type MembershipResult =
  | { kind: "joined"; smartGroupId: string; inkId: string }
  | { kind: "none"; reason: string };

function worldBoundsAsSmartBounds(sg: SmartGroupNode): SmartBounds {
  const w = smartGroupWorldAabb(sg);
  return {
    x: w.minX,
    y: w.minY,
    width: w.maxX - w.minX,
    height: w.maxY - w.minY,
  };
}

/** Paint order: tree walk, later siblings last. */
export function smartGroupsInPaintOrder(nodes: readonly DocNode[]): SmartGroupNode[] {
  const out: SmartGroupNode[] = [];
  const walk = (list: readonly DocNode[]) => {
    for (const n of list) {
      if (n.kind === "smart_group") out.push(n);
      if (n.kind === "frame" || n.kind === "group") walk(n.children);
    }
  };
  walk(nodes);
  return out;
}

/**
 * After ordinary ink is committed, try reparent into a Smart Group.
 * @implements [SRS-IN-15] membership on stroke_end
 */
export function tryDrawIntoMembership(
  tree: VectorDocument,
  undo: UndoRing,
  inkId: string,
): MembershipResult {
  const node = tree.indexById().get(inkId);
  if (!node || node.kind !== "ink") {
    return { kind: "none", reason: "not_ink" };
  }
  // Already under a Smart Group — no-op
  const groups = smartGroupsInPaintOrder(tree.rootChildren);
  for (const sg of groups) {
    if (sg.children.some((c) => c.id === inkId)) {
      return { kind: "none", reason: "already_member" };
    }
  }

  const qualifiers = groups.filter(
    (sg) => fractionSamplesInside(node.samples, worldBoundsAsSmartBounds(sg)) >= 0.8,
  );
  if (qualifiers.length === 0) {
    return { kind: "none", reason: "no_qualifying_group" };
  }
  const winner = qualifiers[qualifiers.length - 1];

  const result = undo.applyWithUndo(tree, {
    opId: `join_smart_group:${inkId}:${winner.id}:${Date.now()}`,
    type: "join_smart_group",
    payload: { inkId, smartGroupId: winner.id },
  });
  if (!result.applied) {
    return { kind: "none", reason: result.reason ?? "join_failed" };
  }
  return { kind: "joined", smartGroupId: winner.id, inkId };
}

/** Seed UV for an ink against a group's local bounds (used by op). */
export function prepareContentForGroup(ink: InkNode, sg: SmartGroupNode): InkNode {
  const next = JSON.parse(JSON.stringify(ink)) as InkNode;
  next.role = "content";
  next.layoutOffset = seedLayoutOffset(next.samples, sg.bounds);
  return next;
}
