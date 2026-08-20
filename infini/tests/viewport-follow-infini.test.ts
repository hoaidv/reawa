/**
 * STORY-IN-037 / viewport-follow-infini.feature
 * @implements [SRS-IN-26] Infini follow Epaper
 * @implements [SRS-IN-27] FollowToggle
 * @implements [SRS-IN-28] follow quality
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { panByScreenDelta } from "../src/canvas/Viewport";
import { VectorDocument } from "../src/document";
import {
  FOLLOW_COPY,
  FOLLOW_P95_MS,
  MemoryTransport,
  TabletSession,
  dualFollowOn,
  followToggleView,
  type ViewportMessage,
} from "../src/session";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

function src(rel: string): string {
  return readFileSync(join(root, rel), "utf8");
}

function liveSession() {
  const tree = new VectorDocument();
  const transport = new MemoryTransport();
  const logs: Array<{ msg: string; detail?: unknown }> = [];
  const session = new TabletSession({
    tree,
    transport,
    cssWidth: 800,
    cssHeight: 600,
    log: (msg, detail) => logs.push({ msg, detail }),
  });
  session.connect();
  return { tree, transport, session, logs };
}

function tabletViewport(
  translate: { x: number; y: number },
  scale: number,
  extra?: Partial<ViewportMessage>,
): ViewportMessage {
  return {
    type: "viewport",
    translate,
    scale,
    drawingRegion: { minX: 0, minY: 0, maxX: 200, maxY: 150 },
    seq: extra?.seq ?? 1,
    settle: extra?.settle ?? true,
    source: extra?.source ?? "epaper",
    ...extra,
  };
}

function docTypes(outbound: { type: string }[]): string[] {
  return outbound.map((m) => m.type).filter((t) => t.startsWith("doc_"));
}

describe("viewport-follow-infini.feature FollowToggle chrome (SRS-IN-27)", () => {
  it("btn.viewport_follow is a desktop icon toggle not a ToolChip and not a child of WorldLayer", () => {
    const toggle = src("src/canvas/FollowToggle.tsx");
    const stage = src("src/canvas/CanvasStage.tsx");
    const app = src("src/App.tsx");
    const css = src("src/styles/app.css");
    expect(toggle).toContain('data-control="btn.viewport_follow"');
    expect(toggle).toContain("c-follow-toggle");
    expect(toggle).not.toMatch(/data-region=["']ToolChip["']/);
    expect(toggle).not.toContain("c-tool-chip");
    expect(stage).not.toMatch(/data-region=["']ToolChip["']/);
    expect(toggle).toContain('data-region="FollowToggle"');
    expect(app).toContain('data-region="WindowFrame"');
    expect(stage).toContain("<FollowToggle");
    expect(stage).toContain('data-region="WorldLayer"');
    // FollowToggle is a fragment sibling of CanvasStage, not nested in the canvas WorldLayer.
    const worldOpen = stage.indexOf("<canvas");
    const worldClose = stage.indexOf("</canvas>");
    const followUse = stage.indexOf("<FollowToggle");
    expect(worldOpen).toBeGreaterThan(-1);
    expect(followUse).toBeGreaterThan(worldClose);
    expect(css).toMatch(/\.c-follow-toggle:hover/);
    expect(css).toContain("--follow-hit: 32px");
  });
});

describe("Scenario: Creator turns Infini follow on from both-off", () => {
  it("emits viewport_follow epaper_to_infini, applies tablet viewport after settle, 0 doc_*", () => {
    const { session, transport } = liveSession();
    expect(session.followDirection).toBe("none");
    const view0 = session.followView();
    expect(view0.ui).toBe("follow.off");
    expect(view0.pressed).toBe(false);
    expect(view0.caption).toBe(FOLLOW_COPY.captionOff);

    const tablet = { x: -80, y: 40 };
    session.receiveTabletViewport(tabletViewport(tablet, 0.87, { settle: true }));
    expect(session.lastAppliedTabletViewport).toBeNull();

    const loads = transport.docLoads.length;
    const ops = transport.docOps.length;
    const t0 = performance.now();
    const result = session.clickFollowToggle();
    const elapsed = result?.elapsedMs ?? performance.now() - t0;

    expect(result).not.toBeNull();
    expect(result!.follow).toMatchObject({
      type: "viewport_follow",
      direction: "epaper_to_infini",
    });
    expect(transport.viewportFollows.at(-1)?.direction).toBe("epaper_to_infini");
    expect(session.followDirection).toBe("epaper_to_infini");
    expect(session.followView().ui).toBe("follow.following_epaper");
    expect(session.followView().pressed).toBe(true);
    expect(session.followView().caption).toBe(FOLLOW_COPY.captionOn);
    expect(dualFollowOn(session.followDirection)).toBe(false);
    expect(session.dualOnCount).toBe(0);
    expect(result!.applied).toEqual({ translate: tablet, scale: 0.87 });
    expect(session.lastAppliedTabletViewport).toEqual({ translate: tablet, scale: 0.87 });
    expect(elapsed).toBeLessThanOrEqual(FOLLOW_P95_MS);
    expect(transport.docLoads).toHaveLength(loads);
    expect(transport.docOps).toHaveLength(ops);
    expect(docTypes(transport.outbound)).toHaveLength(0);
  });
});

describe("Scenario: Creator tap on Infini follow takes over from Epaper follow", () => {
  it("sets epaper_to_infini, peer off, 0 dual-on, apply after settle", () => {
    const { session, transport } = liveSession();
    session.receiveViewportFollow({
      type: "viewport_follow",
      direction: "infini_to_epaper",
      seq: 1,
    });
    expect(session.followDirection).toBe("infini_to_epaper");
    expect(session.followView().ui).toBe("follow.peer_following_you");
    expect(session.followView().pressed).toBe(false);
    expect(session.followView().disabled).toBe(false);
    expect(session.followView().caption).toBe(FOLLOW_COPY.captionPeer);

    session.noteTabletViewport({ translate: { x: 12, y: -4 }, scale: 1.1 });
    const t0 = performance.now();
    const result = session.clickFollowToggle();
    expect(performance.now() - t0).toBeLessThanOrEqual(FOLLOW_P95_MS);
    expect(result?.elapsedMs).toBeLessThanOrEqual(FOLLOW_P95_MS);
    expect(session.followDirection).toBe("epaper_to_infini");
    expect(session.followView().ui).toBe("follow.following_epaper");
    expect(session.followView().pressed).toBe(true);
    expect(dualFollowOn(session.followDirection)).toBe(false);
    expect(session.dualOnCount).toBe(0);
    expect(transport.viewportFollows.at(-1)?.direction).toBe("epaper_to_infini");
    expect(result?.applied).toEqual({ translate: { x: 12, y: -4 }, scale: 1.1 });
  });
});

describe("Scenario: Connection lost forces Infini follow off", () => {
  it("forces none, toggle unavailable, 0 apply and 0 doc_* from the drop", () => {
    const { session, transport, logs } = liveSession();
    session.noteTabletViewport({ translate: { x: -80, y: 40 }, scale: 0.87 });
    session.clickFollowToggle();
    expect(session.followDirection).toBe("epaper_to_infini");
    const loads = transport.docLoads.length;
    const ops = transport.docOps.length;

    session.disconnect();
    expect(session.followDirection).toBe("none");
    expect(session.connected).toBe(false);
    const view = session.followView();
    expect(view.ui).toBe("follow.connection_lost");
    expect(view.disabled).toBe(true);
    expect(view.pressed).toBe(false);
    expect(view.caption).toBe(FOLLOW_COPY.captionLost);

    const inbound = session.receiveTabletViewport(
      tabletViewport({ x: 99, y: 99 }, 2, { settle: true, seq: 9 }),
    );
    expect(inbound.applied).toBe(false);
    expect(session.lastAppliedTabletViewport).toEqual({
      translate: { x: -80, y: 40 },
      scale: 0.87,
    });
    expect(logs.some((l) => l.msg.includes("ignore inbound viewport"))).toBe(true);
    expect(transport.docLoads).toHaveLength(loads);
    expect(transport.docOps).toHaveLength(ops);
    expect(session.clickFollowToggle()).toBeNull();
  });
});

describe("Scenario: Reconnect does not restore Infini follow", () => {
  it("hello does not carry last follow.direction; stays off until click", () => {
    const { session } = liveSession();
    session.noteTabletViewport({ translate: { x: -80, y: 40 }, scale: 0.87 });
    session.clickFollowToggle();
    session.disconnect();

    session.connect();
    expect(session.followDirection).toBe("none");
    session.receiveHello({
      type: "hello",
      lastSeq: 0,
      queued: 0,
      direction: "epaper_to_infini",
    } as never);
    expect(session.followDirection).toBe("none");
    const view = session.followView();
    expect(view.ui).toBe("follow.reconnect_stays_off");
    expect(view.disabled).toBe(false);
    expect(view.pressed).toBe(false);
    expect(view.caption).toBe(FOLLOW_COPY.captionReconnect);

    const inbound = session.receiveTabletViewport(
      tabletViewport({ x: 1, y: 2 }, 3, { settle: true, seq: 4 }),
    );
    expect(inbound.applied).toBe(false);
  });
});

describe("Scenario: Creator pan on Infini while following turns follow off", () => {
  it("sets none before local pan applies and ignores further tablet viewport", () => {
    const { session } = liveSession();
    session.noteTabletViewport({ translate: { x: 0, y: 0 }, scale: 1 });
    session.clickFollowToggle();
    expect(session.followView().ui).toBe("follow.following_epaper");
    expect(session.followView().pressed).toBe(true);

    const before = { translate: { x: 10, y: 20 }, scale: 1 };
    const turnedOff = session.noteFollowerLocalNav();
    expect(turnedOff).toBe(true);
    expect(session.followDirection).toBe("none");
    const afterPan = panByScreenDelta(before, 40, -20);
    expect(afterPan.translate).toEqual({ x: 50, y: 0 });
    expect(session.followView().ui).toBe("follow.local_nav_turns_off");
    expect(session.followView().pressed).toBe(false);
    expect(session.followView().caption).toBe(FOLLOW_COPY.captionLocalNav);

    const inbound = session.receiveTabletViewport(
      tabletViewport({ x: -80, y: 40 }, 0.87, { settle: true, seq: 8 }),
    );
    expect(inbound.applied).toBe(false);
  });
});

describe("Scenario: No session leaves the Infini follow toggle unavailable", () => {
  it("connection_lost, 0 follow-on, direction none", () => {
    const tree = new VectorDocument();
    const transport = new MemoryTransport();
    const session = new TabletSession({ tree, transport });
    expect(session.connected).toBe(false);
    const view = session.followView();
    expect(view.ui).toBe("follow.connection_lost");
    expect(view.disabled).toBe(true);
    expect(view.pressed).toBe(false);
    expect(session.followDirection).toBe("none");
    expect(session.clickFollowToggle()).toBeNull();
    expect(followToggleView({ connected: false, direction: "epaper_to_infini", offKind: "default" }).ui).toBe(
      "follow.connection_lost",
    );
  });
});
