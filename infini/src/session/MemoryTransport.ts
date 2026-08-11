/**
 * In-memory session transport for tests / local loopback.
 * @implements [SRS-IN-07] channel sinks
 */

import type { DocOpMessage, SessionTransport, ViewportMessage } from "./types";

export class MemoryTransport implements SessionTransport {
  readonly viewports: ViewportMessage[] = [];
  readonly docOps: DocOpMessage[] = [];

  /** Optional Epaper-side map apply hook (SRS-IN-08 latency). */
  onViewportApply?: (msg: ViewportMessage, appliedAtMs: number) => void;

  sendViewport(msg: ViewportMessage): void {
    this.viewports.push(msg);
    // Simulate Epaper map apply on receive (sync stub — production is peer).
    const appliedAt = performance.now();
    this.onViewportApply?.(msg, appliedAt);
  }

  sendDocOp(msg: DocOpMessage): void {
    this.docOps.push(msg);
  }
}
