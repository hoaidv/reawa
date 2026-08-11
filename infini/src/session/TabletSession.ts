/**
 * Infini side of ADR-0009 tablet session.
 * @implements [SRS-IN-07] session roles and channel binding
 * @implements [SRS-IN-08] viewport emit timing hook for map-apply budget
 */

import type { InfiniDocument } from "../canvas/Document";
import {
  frameWorldAabb,
  tabletDrawingFrameCss,
  type CssRect,
  type TabletOrientation,
  type Viewport,
  visibleWorldAabb,
} from "../canvas/Viewport";
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

/** Max outbound viewport Hz during gesture coalesce. @implements [SRS-IN-07] */
export const VIEWPORT_PUBLISH_MAX_HZ = 30;
const MIN_PUBLISH_INTERVAL_MS = 1000 / VIEWPORT_PUBLISH_MAX_HZ;

export interface TabletSessionOptions {
  tree: VectorDocument;
  /** WorldLayer host — synced after successful doc apply. */
  world?: InfiniDocument;
  transport: SessionTransport;
  cssWidth?: number;
  cssHeight?: number;
  orientation?: TabletOrientation;
  /** Logger for unknown ops / degrade (does not throw). */
  log?: (msg: string, detail?: unknown) => void;
  /** Inject clock for coalesce tests (ms). */
  nowMs?: () => number;
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
  private orientation: TabletOrientation;
  private readonly log: (msg: string, detail?: unknown) => void;
  private readonly nowMs: () => number;
  private lastPublishAtMs = -Infinity;
  private pendingVp: Viewport | null = null;

  /** Last viewport emit timestamp (ms, performance.now) for latency tests. */
  lastViewportEmitAtMs = 0;

  /** Last emitted message (tests / debug). */
  lastViewportMessage: ViewportMessage | null = null;

  constructor(opts: TabletSessionOptions) {
    this.tree = opts.tree;
    this.world = opts.world;
    this.transport = opts.transport;
    this.cssWidth = opts.cssWidth ?? 800;
    this.cssHeight = opts.cssHeight ?? 600;
    this.orientation = opts.orientation ?? "gutToLeft";
    this.log = opts.log ?? ((m, d) => console.warn(m, d));
    this.nowMs = opts.nowMs ?? (() => performance.now());
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

  setOrientation(o: TabletOrientation): void {
    this.orientation = o;
  }

  getOrientation(): TabletOrientation {
    return this.orientation;
  }

  /** Current tablet CSS frame for marker + drawingRegion. */
  tabletFrame(): CssRect {
    return tabletDrawingFrameCss(this.cssWidth, this.cssHeight, this.orientation);
  }

  /**
   * Publish viewport after Infini pan/zoom.
   * Coalesces to ≤30 Hz unless `force` (gesture settle flush).
   * @implements [SRS-IN-07] viewport message emit + coalesce
   */
  publishViewport(
    vp: Viewport,
    opts?: { force?: boolean; settle?: boolean },
  ): ViewportMessage | null {
    if (!this.connected) return null;
    const force = opts?.force === true;
    const now = this.nowMs();
    if (!force && now - this.lastPublishAtMs < MIN_PUBLISH_INTERVAL_MS) {
      this.pendingVp = vp;
      return null;
    }
    return this.emitViewport(vp, now, opts?.settle === true);
  }

  /**
   * Flush settle pose after gesture end (always emit latest with settle:true).
   * @implements [SRS-IN-07] settle flush
   * @fix [BUG] soft coalesced refreshes leave tablet ghosts without settle
   */
  flushViewport(vp?: Viewport): ViewportMessage | null {
    let next: Viewport | null = vp ?? this.pendingVp;
    if (!next && this.lastViewportMessage) {
      next = {
        translate: { ...this.lastViewportMessage.translate },
        scale: this.lastViewportMessage.scale,
      };
    }
    this.pendingVp = null;
    if (!next || !this.connected) return null;
    return this.emitViewport(next, this.nowMs(), true);
  }

  private emitViewport(vp: Viewport, now: number, settle = false): ViewportMessage {
    this.viewportSeq += 1;
    this.pendingVp = null;
    this.lastPublishAtMs = now;
    const frame = this.tabletFrame();
    const msg: ViewportMessage = {
      type: "viewport",
      translate: { x: vp.translate.x, y: vp.translate.y },
      scale: vp.scale,
      drawingRegion: frameWorldAabb(frame, vp),
      seq: this.viewportSeq,
      orientation: this.orientation,
      settle: settle || undefined,
    };
    this.lastViewportEmitAtMs = now;
    this.lastViewportMessage = msg;
    this.transport.sendViewport(msg);
    return msg;
  }

  /** Full-window world AABB (debug / compare only). */
  fullWindowWorldAabb(vp: Viewport) {
    return visibleWorldAabb(this.cssWidth, this.cssHeight, vp);
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
      return { applied: false, reason: result.reason };
    }

    if (!result.applied && result.reason === "duplicate_opId") {
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
