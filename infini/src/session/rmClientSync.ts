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

/** True when Infini should push doc_snapshot + viewport after observing RM clients. */
export function shouldPublishOnRmClient(ev: RmClientEvent): boolean {
  return (ev.type === "connected" || ev.type === "sync") && ev.n > 0;
}
