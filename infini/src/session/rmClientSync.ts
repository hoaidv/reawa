/**
 * RM client connection UI helpers — STORY-IN-019 @implements [SRS-IN-07] eager sync
 */
export type RmClientEvent = { type: "connected" | "closed" | "sync"; n: number };

export function rmClientSyncHint(ev: RmClientEvent): string {
  if (ev.type === "closed") {
    return ev.n > 0 ? `RM connected (n=${ev.n})` : "RM disconnected — waiting";
  }
  if (ev.n > 0) return `RM connected (n=${ev.n})`;
  return "RM disconnected — waiting";
}

/** Viewport-only on RM connect — document load is IN-028. */
export function shouldPublishViewportOnRmClient(ev: RmClientEvent): boolean {
  return (ev.type === "connected" || ev.type === "sync") && ev.n > 0;
}

/** @deprecated snapshot push withdrawn (STORY-IN-027 / ADR-0015). Always false. */
export function shouldPublishOnRmClient(_ev: RmClientEvent): boolean {
  return false;
}
