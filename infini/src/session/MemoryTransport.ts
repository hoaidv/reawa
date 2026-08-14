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
  ViewportMessage,
} from "./types";

export class MemoryTransport implements SessionTransport {
  readonly viewports: ViewportMessage[] = [];
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
