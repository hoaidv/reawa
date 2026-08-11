/**
 * Pickables + tool_intent helpers for SRS-IN-13.
 * @implements [SRS-IN-13] tool intent transport
 */

import { smartGroupWorldAabb } from "./anchors";
import type { DocNode, SmartBounds, SmartTransform } from "./types";
import type { VectorDocument } from "./VectorDocument";
import type { UndoRing } from "./UndoRing";

export interface Pickable {
  id: string;
  kind: "smart_group";
  bounds: { minX: number; minY: number; maxX: number; maxY: number };
}

export type ToolIntentAction = "select" | "move" | "resize";

export interface ToolIntentMessage {
  type: "tool_intent";
  action: ToolIntentAction;
  nodeId: string;
  delta?: { dx: number; dy: number };
  bounds?: SmartBounds;
  seq?: number;
}

/** Collect SmartGroup pickables in paint order (later = topmost). */
export function buildPickables(tree: VectorDocument): Pickable[] {
  const out: Pickable[] = [];
  const walk = (nodes: readonly DocNode[]) => {
    for (const n of nodes) {
      if (n.kind === "smart_group") {
        const box = smartGroupWorldAabb(n);
        out.push({
          id: n.id,
          kind: "smart_group",
          bounds: {
            minX: box.minX,
            minY: box.minY,
            maxX: box.maxX,
            maxY: box.maxY,
          },
        });
      }
      if (n.kind === "frame" || n.kind === "group") walk(n.children);
    }
  };
  walk(tree.rootChildren);
  return out;
}

export function normalizeStrokeIntent(
  intent: string | undefined | null,
): "ink" | "enclose" {
  if (intent === "enclose") return "enclose";
  return "ink";
}

/**
 * Apply Epaper tool_intent on Infini (authority).
 * @implements [SRS-IN-13] tool_intent apply
 */
export function applyToolIntent(
  tree: VectorDocument,
  undo: UndoRing,
  msg: ToolIntentMessage,
): { applied: boolean; reason?: string } {
  if (msg.type !== "tool_intent") {
    return { applied: false, reason: "not_tool_intent" };
  }
  const node = tree.indexById().get(msg.nodeId);
  if (!node || node.kind !== "smart_group") {
    return { applied: false, reason: "unknown_nodeId" };
  }

  if (msg.action === "select") {
    // Selection is UI state — document unchanged.
    return { applied: true, reason: "select_ui_only" };
  }

  if (msg.action === "move") {
    const dx = msg.delta?.dx ?? 0;
    const dy = msg.delta?.dy ?? 0;
    const transform: SmartTransform = {
      ...node.transform,
      x: node.transform.x + dx,
      y: node.transform.y + dy,
    };
    const r = undo.applyWithUndo(tree, {
      opId: `tool_intent_move:${msg.nodeId}:${msg.seq ?? Date.now()}`,
      type: "set_smart_transform",
      source: "epaper",
      payload: { id: msg.nodeId, transform },
    });
    return { applied: r.applied, reason: r.reason };
  }

  if (msg.action === "resize" && msg.bounds) {
    const r = undo.applyWithUndo(tree, {
      opId: `tool_intent_resize:${msg.nodeId}:${msg.seq ?? Date.now()}`,
      type: "set_smart_transform",
      source: "epaper",
      payload: {
        id: msg.nodeId,
        transform: { ...node.transform },
        bounds: msg.bounds,
      },
    });
    return { applied: r.applied, reason: r.reason };
  }

  return { applied: false, reason: "bad_action" };
}

/** Assert helper: outbound payloads must not carry tool mode. */
export function messageCarriesToolMode(msg: Record<string, unknown>): boolean {
  return (
    "tool" in msg ||
    "toolMode" in msg ||
    "tool_mode" in msg ||
    msg.type === "tool_mode"
  );
}
