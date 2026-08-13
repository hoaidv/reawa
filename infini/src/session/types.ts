/**
 * Tablet session message shapes (JSON-lines framing).
 * @implements [SRS-IN-07] viewport + doc_change envelopes
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
  /** @implements [SRS-IN-13] pickables for device local hit-test */
  pickables?: Array<{
    id: string;
    kind: "smart_group";
    bounds: { minX: number; minY: number; maxX: number; maxY: number };
  }>;
}

/** Epaper → Infini manipulation intent (pilot). */
export interface ToolIntentWireMessage {
  type: "tool_intent";
  action: "select" | "move" | "resize";
  nodeId: string;
  delta?: { dx: number; dy: number };
  bounds?: { x: number; y: number; width: number; height: number };
  seq?: number;
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

/** Nested op inside a doc_change envelope (SRS-IN-09). */
export interface DocChangeOp {
  type: string;
  payload?: Record<string, unknown>;
  opId?: string;
  ts?: number;
  source?: "epaper" | "infini";
}

/**
 * Epaper → Infini committed change (ADR-0015 §2).
 * @implements [SRS-IN-07] doc_change envelope
 */
export interface DocChangeMessage {
  type: "doc_change";
  seq: number;
  opId: string;
  op: DocChangeOp;
  baseSeq: number;
}

export type SessionOutbound = ViewportMessage | DocOpMessage;

/** Peer transport — tests use in-memory; production wires TCP/JSON-lines later. */
export interface SessionTransport {
  sendViewport(msg: ViewportMessage): void;
  sendDocOp(msg: DocOpMessage): void;
}

/**
 * Normalize a doc_change envelope to a DocOp for VectorDocument.applyOp.
 * @implements [SRS-IN-07] doc_change → op
 */
export function docChangeToOp(msg: DocChangeMessage): DocOp {
  const raw = msg.op ?? {};
  const type = String(raw.type ?? "");
  const opId = String(raw.opId ?? msg.opId);
  let payload: Record<string, unknown>;
  if (raw.payload && typeof raw.payload === "object" && !Array.isArray(raw.payload)) {
    payload = raw.payload;
  } else {
    const rest = { ...(raw as unknown as Record<string, unknown>) };
    delete rest.type;
    delete rest.opId;
    delete rest.ts;
    delete rest.source;
    delete rest.payload;
    payload = rest;
  }
  return {
    opId,
    type,
    payload,
    ts: raw.ts,
    source: raw.source ?? "epaper",
  };
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
