/**
 * Infini side of ADR-0009 tablet session.
 * @implements [SRS-IN-07] session roles and channel binding
 * @implements [SRS-IN-08] viewport emit timing hook for map-apply budget
 */

import type { InfiniDocument } from "../canvas/Document";
import { visibleWorldAabb, type Viewport } from "../canvas/Viewport";
import type { VectorDocument } from "../document/VectorDocument";
import type { DocOp } from "../document/types";
import {
  docOpToMessage,
  isInfiniStructureOp,
  messageToDocOp,
  type DocOpMessage,
  type SessionTransport,
  type ViewportMessage,
} from "./types";

export interface TabletSessionOptions {
  tree: VectorDocument;
  /** WorldLayer host — synced after successful doc apply. */
  world?: InfiniDocument;
  transport: SessionTransport;
  cssWidth?: number;
  cssHeight?: number;
  /** Logger for unknown ops / degrade (does not throw). */
  log?: (msg: string, detail?: unknown) => void;
}

export class TabletSession {
  connected = false;
  private viewportSeq = 0;
  private epaperStrokeInFlight = false;
  private structureQueue: DocOp[] = [];
  private readonly tree: VectorDocument;
  private readonly world?: InfiniDocument;
  private readonly transport: SessionTransport;
  private cssWidth: number;
  private cssHeight: number;
  private readonly log: (msg: string, detail?: unknown) => void;

  /** Last viewport emit timestamp (ms, performance.now) for latency tests. */
  lastViewportEmitAtMs = 0;

  constructor(opts: TabletSessionOptions) {
    this.tree = opts.tree;
    this.world = opts.world;
    this.transport = opts.transport;
    this.cssWidth = opts.cssWidth ?? 800;
    this.cssHeight = opts.cssHeight ?? 600;
    this.log = opts.log ?? ((m, d) => console.warn(m, d));
  }

  connect(): void {
    this.connected = true;
  }

  disconnect(): void {
    this.connected = false;
  }

  setCssSize(w: number, h: number): void {
    this.cssWidth = w;
    this.cssHeight = h;
  }

  /**
   * Publish viewport after Infini pan/zoom.
   * @implements [SRS-IN-07] viewport message emit
   */
  publishViewport(vp: Viewport): ViewportMessage | null {
    if (!this.connected) return null;
    this.viewportSeq += 1;
    const msg: ViewportMessage = {
      type: "viewport",
      translate: { x: vp.translate.x, y: vp.translate.y },
      scale: vp.scale,
      drawingRegion: visibleWorldAabb(this.cssWidth, this.cssHeight, vp),
      seq: this.viewportSeq,
    };
    this.lastViewportEmitAtMs = performance.now();
    this.transport.sendViewport(msg);
    return msg;
  }

  /** Epaper reports stroke still capturing samples. */
  setEpaperStrokeInFlight(inFlight: boolean): void {
    this.epaperStrokeInFlight = inFlight;
    if (!inFlight) this.flushStructureQueue();
  }

  get isEpaperStrokeInFlight(): boolean {
    return this.epaperStrokeInFlight;
  }

  get queuedStructureCount(): number {
    return this.structureQueue.length;
  }

  /**
   * Infini wants to emit a structure / Smart Group op (emit matrix).
   * Queues while Epaper stroke is in flight.
   * @implements [SRS-IN-07] no race in-flight Epaper stroke
   */
  emitLocalStructureOp(op: DocOp): { emitted: boolean; queued: boolean } {
    if (!this.connected) return { emitted: false, queued: false };
    const withSource: DocOp = { ...op, source: op.source ?? "infini" };
    if (!isInfiniStructureOp(withSource.type)) {
      this.log("not an Infini structure op", withSource.type);
      return { emitted: false, queued: false };
    }
    // Apply locally first so tree stays ahead of wire.
    const result = this.tree.applyOp(withSource);
    if (!result.applied && result.reason !== "duplicate_opId") {
      this.log("local structure apply failed", result.reason);
      return { emitted: false, queued: false };
    }
    this.syncWorld();

    if (this.epaperStrokeInFlight) {
      this.structureQueue.push(withSource);
      return { emitted: false, queued: true };
    }
    this.transport.sendDocOp(docOpToMessage(withSource));
    return { emitted: true, queued: false };
  }

  /**
   * Receive a document-channel op from Epaper (or peer).
   * @implements [SRS-IN-07] idempotent apply + unknown type log
   */
  receiveDocOp(msg: DocOpMessage): {
    applied: boolean;
    reason?: string;
  } {
    if (!this.connected) return { applied: false, reason: "not_connected" };
    const op = messageToDocOp(msg);
    const before = this.tree.snapshotString();
    const result = this.tree.applyOp(op);

    if (result.reason?.startsWith("unknown_type:")) {
      this.log("unknown doc_op type", { type: op.type, opId: op.opId });
      // Tree unchanged — applyOp does not mutate on unknown
      return { applied: false, reason: result.reason };
    }

    if (!result.applied && result.reason === "duplicate_opId") {
      // Idempotent — tree must match prior
      if (this.tree.snapshotString() !== before) {
        this.log("idempotent apply mutated tree unexpectedly", op.opId);
      }
      return { applied: false, reason: "duplicate_opId" };
    }

    if (result.applied) this.syncWorld();
    return result;
  }

  private flushStructureQueue(): void {
    while (this.structureQueue.length > 0 && !this.epaperStrokeInFlight) {
      const op = this.structureQueue.shift()!;
      this.transport.sendDocOp(docOpToMessage(op));
    }
  }

  private syncWorld(): void {
    this.world?.syncFromVectorDoc(this.tree);
  }
}
