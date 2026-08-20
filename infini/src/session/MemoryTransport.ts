/**
 * In-memory session transport for tests / local loopback.
 * @implements [SRS-IN-07] channel sinks
 */

import type {
  DocLoadMessage,
  DocOpMessage,
  DrainAckMessage,
  SessionOutbound,
  SessionTransport,
  ViewportFollowMessage,
  ViewportMessage,
} from "./types";

export class MemoryTransport implements SessionTransport {
  readonly viewports: ViewportMessage[] = [];
  readonly viewportFollows: ViewportFollowMessage[] = [];
  readonly drainAcks: DrainAckMessage[] = [];
  readonly docLoads: DocLoadMessage[] = [];
  readonly docOps: DocOpMessage[] = [];
  readonly outbound: SessionOutbound[] = [];

  /** Optional Epaper-side map apply hook (SRS-IN-08 latency). */
  onViewportApply?: (msg: ViewportMessage, appliedAtMs: number) => void;

  sendViewport(msg: ViewportMessage): void {
    this.viewports.push(msg);
    this.outbound.push(msg);
    const appliedAt = performance.now();
    this.onViewportApply?.(msg, appliedAt);
  }

  sendViewportFollow(msg: ViewportFollowMessage): void {
    this.viewportFollows.push(msg);
    this.outbound.push(msg);
  }

  sendDrainAck(msg: DrainAckMessage): void {
    this.drainAcks.push(msg);
    this.outbound.push(msg);
  }

  sendDocLoad(msg: DocLoadMessage): void {
    this.docLoads.push(msg);
    this.outbound.push(msg);
  }

  sendDocOp(msg: DocOpMessage): void {
    this.docOps.push(msg);
    this.outbound.push(msg);
  }
}
