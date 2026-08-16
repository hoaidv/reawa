/**
 * Tablet session message shapes (JSON-lines framing).
 * @implements [SRS-IN-07] viewport + handshake + doc_change envelopes
 * @implements [SRS-IN-09] transmit names hello drain_ack doc_load
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

/**
 * Infini → Epaper handshake-gated load (replaces retired doc_snapshot).
 * @implements [SRS-IN-07] doc_load envelope
 * @implements [SRS-IN-09] document-load envelope
 */
export interface DocLoadMessage {
  type: "doc_load";
  document: Record<string, unknown>;
  seq: 0;
}

/**
 * Infini → Epaper: device may flush its queued doc_change stream.
 * @implements [SRS-IN-07] drain_ack
 */
export interface DrainAckMessage {
  type: "drain_ack";
}

/** Epaper → Infini session hello. @implements [SRS-IN-07] hello lastSeq queued */
export interface HelloMessage {
  type: "hello";
  lastSeq: number;
  queued: number;
  /** Present when queued is 0 and the device already has a tree (reconnect). */
  document?: Record<string, unknown>;
}

/** Epaper → Infini: drain complete. @implements [SRS-IN-07] queue_empty */
export interface QueueEmptyMessage {
  type: "queue_empty";
}

/** Epaper → Infini: load applied. @implements [SRS-IN-07] load_ack */
export interface LoadAckMessage {
  type: "load_ack";
}

/** Retired wire shape — do not emit. Kept for type-level rejection tests. */
export interface DocSnapshotMessage {
  type: "doc_snapshot";
  nodes: Array<Record<string, unknown>>;
  pickables?: Array<{
    id: string;
    kind: "smart_group";
    bounds: { minX: number; minY: number; maxX: number; maxY: number };
  }>;
}

/** Retired wire shape — do not emit. */
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

export type SessionOutbound =
  | ViewportMessage
  | DocLoadMessage
  | DrainAckMessage
  | DocOpMessage;

/** Peer transport — tests use in-memory; production wires TCP/JSON-lines. */
export interface SessionTransport {
  sendViewport(msg: ViewportMessage): void;
  sendDrainAck(msg: DrainAckMessage): void;
  sendDocLoad(msg: DocLoadMessage): void;
  /** Retired — Infini is a viewer; implementations must no-op. */
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
    opType === "join_smart_group" ||
    opType === "set_smart_transform" ||
    opType === "set_ink_scale_mode" ||
    opType === "recognize_enclose"
  );
}
