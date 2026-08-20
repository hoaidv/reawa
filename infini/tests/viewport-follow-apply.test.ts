/**
 * STORY-IN-033 / viewport-follow-apply.feature
 * @implements [SRS-IN-20] apply tablet viewport while following
 * @implements [SRS-IN-21] viewport emit/apply session gates
 * @implements [SRS-IN-22] tablet-viewport apply quality
 * @implements [SRS-IN-26] follow.direction gate (enum only)
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { panByScreenDelta, zoomAtScreenPoint } from "../src/canvas/Viewport";
import { VectorDocument } from "../src/document";
import {
  MemoryTransport,
  TabletSession,
  worldLayerFromLocalViewport,
  xywhToAabb,
  type ViewportMessage,
} from "../src/session";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

function src(rel: string): string {
  return readFileSync(join(root, rel), "utf8");
}

const CSS_W = 800;
const CSS_H = 600;

function liveSession() {
  const tree = new VectorDocument();
  const transport = new MemoryTransport();
  const logs: Array<{ msg: string; detail?: unknown }> = [];
  const session = new TabletSession({
    tree,
    transport,
    cssWidth: CSS_W,
    cssHeight: CSS_H,
    log: (msg, detail) => logs.push({ msg, detail }),
  });
  session.connect();
  return { tree, transport, session, logs };
}

function followEpaper(session: TabletSession, seq = 1): void {
  session.receiveViewportFollow({
    type: "viewport_follow",
    direction: "epaper_to_infini",
    seq,
  });
}

function xywh(x: number, y: number, w: number, h: number) {
  return { x, y, w, h };
}

function tabletViewport(
  translate: { x: number; y: number },
  scale: number,
  region: { x: number; y: number; w: number; h: number },
  extra?: Partial<ViewportMessage>,
): ViewportMessage {
  return {
    type: "viewport",
    translate,
    scale,
    drawingRegion: xywhToAabb(region),
    seq: extra?.seq ?? 1,
    settle: extra?.settle ?? true,
    source: extra?.source ?? "epaper",
    orientation: extra?.orientation ?? "gutToLeft",
    ...extra,
  };
}

function docTypes(outbound: { type: string }[]): string[] {
  return outbound.map((m) => m.type).filter((t) => t.startsWith("doc_"));
}

describe("viewport-follow-apply.feature (STORY-IN-033)", () => {
  describe("Scenario: Tablet two-finger pan while Infini following matches after settle", () => {
    it("applies translate, drawingRegion, 0 competing down, 0 doc_*, follow stays on", () => {
      const { session, transport } = liveSession();
      followEpaper(session);
      expect(session.followDirection).toBe("epaper_to_infini");
      expect(session.worldLayer.translate).toEqual({ x: 0, y: 0 });
      expect(session.worldLayer.scale).toBe(1);

      const region = xywh(-160, 72, 1404, 1872);
      const inbound = tabletViewport({ x: -160, y: 72 }, 1, region, {
        settle: true,
        seq: 14,
        source: "epaper",
        orientation: "gutToLeft",
      });
      const loads = transport.docLoads.length;
      const ops = transport.docOps.length;
      const downBefore = transport.viewports.length;

      const result = session.receiveTabletViewport(inbound);

      expect(result.applied).toBe(true);
      expect(session.inboundApplyCount).toBe(1);
      expect(session.worldLayer.translate).toEqual({ x: -160, y: 72 });
      expect(session.worldLayer.scale).toBe(1);
      expect(session.worldLayer.drawingRegion).toEqual(region);
      expect(session.lastAppliedDrawingRegion).toEqual(region);
      expect(session.lastAppliedTabletViewport).toEqual({
        translate: { x: -160, y: 72 },
        scale: 1,
      });
      expect(session.publishViewport(session.lastAppliedTabletViewport!, { force: true })).toBeNull();
      expect(transport.viewports).toHaveLength(downBefore);
      expect(transport.docLoads).toHaveLength(loads);
      expect(transport.docOps).toHaveLength(ops);
      expect(docTypes(transport.outbound)).toHaveLength(0);
      expect(session.followDirection).toBe("epaper_to_infini");
    });
  });

  describe("Scenario: Tablet two-finger pinch while Infini following matches uniform scale after settle", () => {
    it("applies uniform scale, 0 rotation-skew, drawingRegion, 0 competing down", () => {
      const { session, transport } = liveSession();
      followEpaper(session);
      session.worldLayer = worldLayerFromLocalViewport(
        { translate: { x: -160, y: 72 }, scale: 1 },
        xywh(-160, 72, 1404, 1872),
      );

      const region = xywh(-160, 72, 1123.2, 1497.6);
      const result = session.receiveTabletViewport(
        tabletViewport({ x: -160, y: 72 }, 1.25, region, {
          settle: true,
          seq: 15,
          source: "epaper",
          orientation: "gutToLeft",
        }),
      );

      expect(result.applied).toBe(true);
      expect(session.worldLayer.translate).toEqual({ x: -160, y: 72 });
      expect(session.worldLayer.scale).toBe(1.25);
      expect(session.worldLayer.scaleX).toBe(session.worldLayer.scaleY);
      expect(session.worldLayer.scaleX).toBe(1.25);
      expect(session.worldLayer.rotation).toBe(0);
      expect(session.worldLayer.skew).toBe(0);
      expect(session.worldLayer.drawingRegion.x).toBeCloseTo(-160);
      expect(session.worldLayer.drawingRegion.y).toBeCloseTo(72);
      expect(session.worldLayer.drawingRegion.w).toBeCloseTo(1123.2);
      expect(session.worldLayer.drawingRegion.h).toBeCloseTo(1497.6);
      expect(session.publishViewport({ translate: { x: -160, y: 72 }, scale: 1.25 }, { force: true })).toBeNull();
      expect(transport.viewports).toHaveLength(0);
      expect(session.followDirection).toBe("epaper_to_infini");
    });
  });

  describe("Scenario: Follow off leaves Infini camera unchanged when the tablet pans", () => {
    it("applies 0 inbound, 0 viewport either way, late inbound logged not implicit follow-on", () => {
      const { session, transport, logs } = liveSession();
      expect(session.followDirection).toBe("none");
      session.worldLayer = worldLayerFromLocalViewport(
        { translate: { x: 12, y: -8 }, scale: 1.1 },
        xywh(12, -8, 1404, 1872),
      );
      const pose = structuredClone(session.worldLayer);
      const applies = session.inboundApplyCount;
      const down = transport.viewports.length;
      const follows = transport.viewportFollows.length;

      const late = tabletViewport({ x: -200, y: 90 }, 0.8, xywh(-200, 90, 1404, 1872), {
        settle: true,
        seq: 40,
        source: "epaper",
      });
      const result = session.receiveTabletViewport(late);

      expect(result.applied).toBe(false);
      expect(session.inboundApplyCount).toBe(applies);
      expect(session.worldLayer).toEqual(pose);
      expect(transport.viewports).toHaveLength(down);
      expect(transport.viewportFollows).toHaveLength(follows);
      expect(session.publishViewport({ translate: { x: 12, y: -8 }, scale: 1.1 }, { force: true })).toBeNull();
      expect(logs.some((l) => l.msg.includes("ignore inbound viewport") && l.detail === "none")).toBe(
        true,
      );
      expect(session.followDirection).toBe("none");
      expect(session.lastAppliedTabletViewport).toBeNull();
    });
  });

  describe("Scenario: Infini as leader ignores inbound tablet viewport", () => {
    it("keeps local camera, logs ignore, 0 implicit follow-on, 0 doc_*", () => {
      const { session, transport, logs } = liveSession();
      session.receiveViewportFollow({
        type: "viewport_follow",
        direction: "infini_to_epaper",
        seq: 1,
      });
      session.worldLayer = worldLayerFromLocalViewport(
        { translate: { x: 20, y: -10 }, scale: 1.2 },
        xywh(20, -10, 1404, 1872),
      );
      const pose = structuredClone(session.worldLayer);
      const applies = session.inboundApplyCount;
      const loads = transport.docLoads.length;
      const ops = transport.docOps.length;

      const result = session.receiveTabletViewport(
        tabletViewport({ x: -200, y: 90 }, 0.8, xywh(-200, 90, 1404, 1872), {
          settle: true,
          seq: 99,
          source: "epaper",
        }),
      );

      expect(result.applied).toBe(false);
      expect(session.inboundApplyCount).toBe(applies);
      expect(session.worldLayer).toEqual(pose);
      expect(logs.some((l) => l.msg.includes("ignore inbound viewport") && l.detail === "infini_to_epaper")).toBe(
        true,
      );
      expect(session.followDirection).toBe("infini_to_epaper");
      expect(transport.docLoads).toHaveLength(loads);
      expect(transport.docOps).toHaveLength(ops);
      expect(docTypes(transport.outbound)).toHaveLength(0);
    });
  });

  describe("Scenario: Infini local pan while following turns follow off and leaves Epaper camera unchanged", () => {
    it("sets none before pan, 0 viewport down, 0 further tablet apply, 0 doc_*", () => {
      const { session, transport } = liveSession();
      followEpaper(session);
      const tablet = { translate: { x: -160, y: 72 }, scale: 1 };
      session.worldLayer = worldLayerFromLocalViewport(tablet, xywh(-160, 72, 1404, 1872));
      const epaper = { ...tablet };
      const applies = session.inboundApplyCount;
      const down = transport.viewports.length;

      const stage = src("src/canvas/CanvasStage.tsx");
      const downAt = stage.indexOf("const onPointerDown");
      const moveAt = stage.indexOf("const onPointerMove");
      const downBlock = stage.slice(downAt, moveAt);
      expect(downBlock).toContain("noteFollowerLocalNav");
      expect(downBlock).not.toContain("panByScreenDelta");
      const moveBlock = stage.slice(moveAt, stage.indexOf("const onPointerUp"));
      expect(moveBlock).toContain("panByScreenDelta");

      const turnedOff = session.noteFollowerLocalNav();
      expect(turnedOff).toBe(true);
      expect(session.followDirection).toBe("none");

      const after = panByScreenDelta(tablet, 48, -24);
      session.worldLayer = worldLayerFromLocalViewport(after, session.worldLayer.drawingRegion);
      expect(session.publishViewport(after, { force: true })).toBeNull();
      expect(transport.viewports).toHaveLength(down);
      expect(session.worldLayer.translate).toEqual(after.translate);
      expect(session.worldLayer.scale).toBe(1);
      expect(epaper).toEqual(tablet);

      const further = session.receiveTabletViewport(
        tabletViewport({ x: -200, y: 90 }, 0.8, xywh(-200, 90, 1404, 1872), {
          settle: true,
          seq: 80,
          source: "epaper",
        }),
      );
      expect(further.applied).toBe(false);
      expect(session.inboundApplyCount).toBe(applies);
      expect(session.worldLayer.translate).toEqual(after.translate);
      expect(docTypes(transport.outbound)).toHaveLength(0);
    });
  });

  describe("Scenario: Infini local pinch while following turns follow off and leaves Epaper camera unchanged", () => {
    it("sets none before pinch, uniform 0.75, 0 viewport down, 0 further tablet apply", () => {
      const { session, transport } = liveSession();
      followEpaper(session);
      const tablet = { translate: { x: -160, y: 72 }, scale: 1 };
      session.worldLayer = worldLayerFromLocalViewport(tablet, xywh(-160, 72, 1404, 1872));
      const epaper = { ...tablet };
      const applies = session.inboundApplyCount;
      const down = transport.viewports.length;

      const stage = src("src/canvas/CanvasStage.tsx");
      const wheelBlockStart = stage.indexOf("const onWheel");
      const wheelBlock = stage.slice(wheelBlockStart, stage.indexOf("host.addEventListener(\"wheel\""));
      expect(wheelBlock.indexOf("noteFollowerLocalNav")).toBeLessThan(wheelBlock.indexOf("zoomAtScreenPoint"));
      expect(wheelBlock.indexOf("noteFollowerLocalNav")).toBeLessThan(wheelBlock.indexOf("panByScreenDelta"));

      const turnedOff = session.noteFollowerLocalNav();
      expect(turnedOff).toBe(true);
      expect(session.followDirection).toBe("none");

      const after = zoomAtScreenPoint(tablet, { x: CSS_W / 2, y: CSS_H / 2 }, 0.75);
      session.worldLayer = worldLayerFromLocalViewport(after, session.worldLayer.drawingRegion);
      expect(session.publishViewport(after, { force: true })).toBeNull();
      expect(session.worldLayer.scale).toBe(0.75);
      expect(session.worldLayer.scaleX).toBe(session.worldLayer.scaleY);
      expect(session.worldLayer.rotation).toBe(0);
      expect(session.worldLayer.skew).toBe(0);
      expect(session.worldLayer.translate).toEqual(after.translate);
      expect(transport.viewports).toHaveLength(down);
      expect(epaper).toEqual(tablet);

      const further = session.receiveTabletViewport(
        tabletViewport({ x: -200, y: 90 }, 0.8, xywh(-200, 90, 1404, 1872), {
          settle: true,
          seq: 81,
          source: "epaper",
        }),
      );
      expect(further.applied).toBe(false);
      expect(session.inboundApplyCount).toBe(applies);
    });
  });
});
