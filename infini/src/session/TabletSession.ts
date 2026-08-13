/**
 * Infini side of ADR-0015 tablet session — viewer + inbound applier.
 * @implements [SRS-IN-07] session roles and channel binding
 * @implements [SRS-IN-08] viewport emit timing + mirror apply budget
 */

import { makePath } from "../canvas/primitives";
import type { InfiniDocument } from "../canvas/Document";
import {
  frameWorldAabb,
  tabletDrawingFrameCss,
  type CssRect,
  type TabletOrientation,
  type Viewport,
  visibleWorldAabb,
} from "../canvas/Viewport";
import { drawablesToPrimitives } from "../document/toPrimitives";
import type { VectorDocument } from "../document/VectorDocument";
import type { DocOp } from "../document/types";
import {
  docChangeToOp,
  messageToDocOp,
  type DocChangeMessage,
  type DocOpMessage,
  type SessionTransport,
  type ViewportMessage,
} from "./types";

/** Max outbound viewport Hz during gesture coalesce. @implements [SRS-IN-07] */
export const VIEWPORT_PUBLISH_MAX_HZ = 30;
const MIN_PUBLISH_INTERVAL_MS = 1000 / VIEWPORT_PUBLISH_MAX_HZ;

export interface TabletSessionOptions {
  tree: VectorDocument;
  /** WorldLayer host — synced from the mirror after apply. */
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

export interface PreviewStroke {
  id: string;
  points: { x: number; y: number }[];
  width: number;
}

export interface ApplyChangeResult {
  applied: boolean;
  reason?: string;
  /** Wall time of apply (ms) for SRS-IN-08 budget. */
  elapsedMs?: number;
  resyncRequested?: boolean;
}

export class TabletSession {
  connected = false;
  /** Last successfully applied doc_change seq. After load epoch this is 0. */
  lastAppliedSeq = 0;
  /** True after a gap or unknown op — do not save, request resync. */
  resyncRequested = false;
  /** Last inbound apply timestamp (ms) for SRS-IN-08. */
  lastApplyAtMs = 0;

  private viewportSeq = 0;
  private epaperStrokeInFlight = false;
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
  private readonly previews = new Map<string, PreviewStroke>();

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
    this.dropAllPreviews();
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

  get mirrorSuspect(): boolean {
    return this.tree.mirrorSuspect;
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
  }

  get isEpaperStrokeInFlight(): boolean {
    return this.epaperStrokeInFlight;
  }

  get queuedStructureCount(): number {
    return 0;
  }

  /**
   * Infini is a viewer — it does not emit document ops (ADR-0014 / SRS-IN-07).
   * @implements [SRS-IN-07] 0 outbound document ops
   */
  emitLocalStructureOp(op: DocOp): { emitted: boolean; queued: boolean; reason?: string } {
    this.log("viewer_only: Infini does not emit document ops", op.type);
    return { emitted: false, queued: false, reason: "viewer_only" };
  }

  /**
   * Apply an inbound `doc_change` into the mirror tree and paint WorldLayer from it.
   * @implements [SRS-IN-07] inbound doc_change applier
   * @implements [SRS-IN-08] device commit → mirror apply budget
   */
  receiveDocChange(msg: DocChangeMessage): ApplyChangeResult {
    const t0 = this.nowMs();
    if (!this.connected) return { applied: false, reason: "not_connected" };
    const op = docChangeToOp(msg);
    const before = this.tree.snapshotString();

    if (this.tree.hasAppliedOpId(op.opId)) {
      if (this.tree.snapshotString() !== before) {
        this.log("idempotent apply mutated tree unexpectedly", op.opId);
      }
      return { applied: false, reason: "duplicate_opId", elapsedMs: this.nowMs() - t0 };
    }

    if (this.tree.mirrorSuspect) {
      this.resyncRequested = true;
      return {
        applied: false,
        reason: "mirror_suspect",
        resyncRequested: true,
        elapsedMs: this.nowMs() - t0,
      };
    }

    if (msg.baseSeq !== this.lastAppliedSeq) {
      this.markSuspectAndResync(
        `gap:baseSeq=${msg.baseSeq} lastAppliedSeq=${this.lastAppliedSeq}`,
      );
      return {
        applied: false,
        reason: "seq_gap",
        resyncRequested: true,
        elapsedMs: this.nowMs() - t0,
      };
    }

    const result = this.tree.applyOp(op);

    if (!result.applied && result.reason === "duplicate_opId") {
      return { applied: false, reason: "duplicate_opId", elapsedMs: this.nowMs() - t0 };
    }

    if (result.reason?.startsWith("unknown_type:")) {
      this.log("unknown doc_change op type", { type: op.type, opId: op.opId, seq: msg.seq });
      this.markSuspectAndResync(`unknown_type:${op.type}`);
      return {
        applied: false,
        reason: result.reason,
        resyncRequested: true,
        elapsedMs: this.nowMs() - t0,
      };
    }

    if (!result.applied) {
      this.log("doc_change apply failed", { reason: result.reason, opId: op.opId, seq: msg.seq });
      this.markSuspectAndResync(result.reason ?? "apply_failed");
      return {
        applied: false,
        reason: result.reason,
        resyncRequested: true,
        elapsedMs: this.nowMs() - t0,
      };
    }

    this.lastAppliedSeq = msg.seq;
    this.lastApplyAtMs = this.nowMs();
    this.dropPreviewsForOp(op);
    this.paintMirror();
    return { applied: true, elapsedMs: this.lastApplyAtMs - t0 };
  }

  /**
   * Receive a legacy `doc_op` (retired wire). Applies without seq/gap — tests only.
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

    if (result.applied) this.paintMirror();
    return result;
  }

  /**
   * WorldLayer from the mirror tree + transient previews (never persisted).
   * Replaces CanvasStage.rebuildWithRmInk as the document paint source.
   * @implements [SRS-IN-07] WorldLayer paints the mirror
   */
  paintMirror(): void {
    if (!this.world) return;
    const treePrims = drawablesToPrimitives(this.tree.flatten());
    const live = [...this.previews.values()]
      .filter((s) => s.points.length >= 2)
      .map((s) =>
        makePath(`preview:${s.id}`, s.points, {
          stroke: "#1C2430",
          strokeWidth: s.width,
        }),
      );
    this.world.setPrimitives([...treePrims, ...live]);
  }

  /**
   * Preview stream — advisory, never written to the mirror.
   * @implements [SRS-IN-07] transient preview path
   */
  previewBegin(id: string, width: number): void {
    this.previews.set(id, { id, points: [], width });
  }

  previewPoint(id: string, x: number, y: number, width = 2.5): void {
    let stroke = this.previews.get(id);
    if (!stroke) {
      stroke = { id, points: [], width };
      this.previews.set(id, stroke);
    }
    stroke.points.push({ x, y });
    this.paintMirror();
  }

  previewEnd(id: string): void {
    // Keep until matching doc_change (SRS-IN-07 lifetime).
    void id;
    this.paintMirror();
  }

  previewIds(): string[] {
    return [...this.previews.keys()];
  }

  dropAllPreviews(): void {
    this.previews.clear();
    this.paintMirror();
  }

  /**
   * Persistence guard — never serialize a gapped mirror.
   * @implements [SRS-IN-07] do not save a suspect mirror
   */
  trySerializeMirror(serialize: (tree: VectorDocument) => string): string | null {
    if (this.tree.mirrorSuspect || !this.tree.canSave()) return null;
    return serialize(this.tree);
  }

  private dropPreviewsForOp(op: DocOp): void {
    for (const id of idsReferencedByOp(op)) {
      this.previews.delete(id);
    }
  }

  private markSuspectAndResync(reason: string): void {
    this.tree.markSuspect(reason);
    this.resyncRequested = true;
    this.log("mirror_suspect: requesting explicit resync", reason);
  }
}

/**
 * Node ids an applied op may replace a preview for (stroke id == node id).
 * @implements [SRS-IN-07] preview replaced by matching doc_change
 */
function idsReferencedByOp(op: DocOp): string[] {
  const ids: string[] = [];
  const p = op.payload;
  if (typeof p.id === "string") ids.push(p.id);
  if (typeof p.inkId === "string") ids.push(p.inkId);
  if (Array.isArray(p.captureIds)) {
    for (const c of p.captureIds) if (typeof c === "string") ids.push(c);
  }
  if (Array.isArray(p.children)) {
    for (const c of p.children) {
      if (c && typeof c === "object" && typeof (c as { id?: unknown }).id === "string") {
        ids.push((c as { id: string }).id);
      }
    }
  }
  return ids;
}
