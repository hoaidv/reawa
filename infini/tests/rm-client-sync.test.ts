/**
 * STORY-IN-019 / @SRS-IN-07 — RM connection eager sync on load.
 */
import { describe, expect, it } from "vitest";
import { rmClientSyncHint, shouldPublishOnRmClient, shouldPublishViewportOnRmClient } from "../src/session/rmClientSync";

describe("SRS-IN-07 RM client eager sync", () => {
  it("does not publish a document snapshot (withdrawn IN-027)", () => {
    expect(shouldPublishOnRmClient({ type: "sync", n: 1 })).toBe(false);
    expect(shouldPublishOnRmClient({ type: "connected", n: 1 })).toBe(false);
  });

  it("still flushes viewport when a client is present", () => {
    expect(shouldPublishViewportOnRmClient({ type: "sync", n: 1 })).toBe(true);
    expect(shouldPublishViewportOnRmClient({ type: "connected", n: 1 })).toBe(true);
    expect(shouldPublishViewportOnRmClient({ type: "sync", n: 0 })).toBe(false);
    expect(shouldPublishViewportOnRmClient({ type: "closed", n: 0 })).toBe(false);
  });

  it("maps client events to sync hint strings", () => {
    expect(rmClientSyncHint({ type: "sync", n: 1 })).toBe("RM connected (n=1)");
    expect(rmClientSyncHint({ type: "sync", n: 0 })).toBe("RM disconnected — waiting");
    expect(rmClientSyncHint({ type: "closed", n: 0 })).toBe("RM disconnected — waiting");
    expect(rmClientSyncHint({ type: "closed", n: 2 })).toBe("RM connected (n=2)");
  });
});
