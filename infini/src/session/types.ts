/**
 * Tablet session message shapes (JSON-lines framing).
 * @implements [SRS-IN-07] viewport + doc_op envelopes
 */

import type { Aabb, TabletOrientation } from "../canvas/Viewport";
import type { DocOp } from "../document/types";

export interface ViewportMessage {
  type: "viewport";
  translate: { x: number; y: number };
  scale: number;
  drawingRegion: Aabb;
  seq: number;
  /** Sync-frame orientation; tablet maps pen + refresh accordingly. */
  orientation?: TabletOrientation;
  /** True on gesture settle — Epaper should sharp-rasterize vectors. */
  settle?: boolean;
}

/** Infini → Epaper initial / authoritative vector snapshot (not a bitmap). */
export interface DocSnapshotMessage {
  type: "doc_snapshot";
  nodes: Array<Record<string, unknown>>;
}

export interface DocOpMessage {
  type: "doc_op";
  /** Envelope matches SRS-IN-09 / SRS-IN-07. */
  opId: string;
  opType: string;
  payload: Record<string, unknown>;
  source: "epaper" | "infini";
  ts?: number;
}

export type SessionOutbound = ViewportMessage | DocOpMessage;

/** Peer transport — tests use in-memory; production wires TCP/JSON-lines later. */
export interface SessionTransport {
  sendViewport(msg: ViewportMessage): void;
  sendDocOp(msg: DocOpMessage): void;
}

export function docOpToMessage(op: DocOp): DocOpMessage {
  return {
    type: "doc_op",
    opId: op.opId,
    opType: op.type,
    payload: op.payload,
    source: op.source ?? "infini",
    ts: op.ts,
  };
}

export function messageToDocOp(msg: DocOpMessage): DocOp {
  return {
    opId: msg.opId,
    type: msg.opType,
    payload: msg.payload,
    source: msg.source,
    ts: msg.ts,
  };
}

/** Structure / Smart Group ops Infini may emit in v0 (emit matrix). */
export function isInfiniStructureOp(opType: string): boolean {
  return (
    opType.startsWith("create_") ||
    opType === "insert_node" ||
    opType === "reparent" ||
    opType === "remove_node" ||
    opType === "set_smart_transform" ||
    opType === "set_ink_scale_mode" ||
    opType === "recognize_enclose"
  );
}
