import { describe, expect, it } from "vitest";
import {
  frameUvToPanel,
  frameWorldAabb,
  identityViewport,
  panelToFrameUv,
  panByScreenDelta,
  preserveCenterOnResize,
  screenToWorld,
  TABLET_ORIENTATIONS,
  tabletDrawingFrameCss,
  worldToScreen,
  zoomAtScreenPoint,
  visibleWorldAabb,
} from "../src/canvas/Viewport";
import { InfiniDocument } from "../src/canvas/Document";
import { makeEllipse, makePath, makeRect } from "../src/canvas/primitives";

describe("Viewport SRS-IN-01 transform", () => {
  it("round-trips world ↔ screen", () => {
    const vp = { translate: { x: 10, y: -5 }, scale: 2 };
    const w = { x: 3, y: 7 };
    const s = worldToScreen(w, vp);
    expect(s).toEqual({ x: (3 + 10) * 2, y: (7 - 5) * 2 });
    expect(screenToWorld(s, vp).x).toBeCloseTo(w.x);
    expect(screenToWorld(s, vp).y).toBeCloseTo(w.y);
  });

  it("keeps circle aspect under uniform scale (programmatic)", () => {
    const vp0 = identityViewport();
    const vp = zoomAtScreenPoint(vp0, { x: 100, y: 100 }, 1.5);
    expect(vp.scale).toBe(1.5);
    // Uniform: screen dx/dy for equal world deltas stay equal.
    const a = worldToScreen({ x: 0, y: 0 }, vp);
    const b = worldToScreen({ x: 10, y: 0 }, vp);
    const c = worldToScreen({ x: 0, y: 10 }, vp);
    expect(Math.hypot(b.x - a.x, b.y - a.y)).toBeCloseTo(
      Math.hypot(c.x - a.x, c.y - a.y),
    );
  });

  it("zoom keeps focal world point fixed", () => {
    const vp0 = { translate: { x: 0, y: 0 }, scale: 1 };
    const focus = { x: 200, y: 150 };
    const worldBefore = screenToWorld(focus, vp0);
    const vp1 = zoomAtScreenPoint(vp0, focus, 2);
    const worldAfter = screenToWorld(focus, vp1);
    expect(worldAfter.x).toBeCloseTo(worldBefore.x);
    expect(worldAfter.y).toBeCloseTo(worldBefore.y);
  });

  it("pan moves content with screen delta", () => {
    const vp = panByScreenDelta(identityViewport(), 40, -20);
    expect(vp.translate.x).toBeCloseTo(40);
    expect(vp.translate.y).toBeCloseTo(-20);
  });

  it("resize preserves world under center", () => {
    const vp = { translate: { x: 0, y: 0 }, scale: 1 };
    const centerWorld = screenToWorld({ x: 400, y: 300 }, vp);
    const next = preserveCenterOnResize(vp, 800, 600, 1000, 500);
    const after = screenToWorld({ x: 500, y: 250 }, next);
    expect(after.x).toBeCloseTo(centerWorld.x);
    expect(after.y).toBeCloseTo(centerWorld.y);
  });
});

describe("SRS-IN-07 tablet drawing frame", () => {
  it("frameWorldAabb matches screenToWorld corners", () => {
    const frame = tabletDrawingFrameCss(800, 600);
    const vp = identityViewport();
    const aabb = frameWorldAabb(frame, vp);
    const tl = screenToWorld({ x: frame.x, y: frame.y }, vp);
    const br = screenToWorld({ x: frame.x + frame.w, y: frame.y + frame.h }, vp);
    expect(aabb.minX).toBeCloseTo(Math.min(tl.x, br.x));
    expect(aabb.maxX).toBeCloseTo(Math.max(tl.x, br.x));
    expect(aabb.minY).toBeCloseTo(Math.min(tl.y, br.y));
    expect(aabb.maxY).toBeCloseTo(Math.max(tl.y, br.y));
  });

  it("maximizes one axis to the host viewport", () => {
    const frame = tabletDrawingFrameCss(800, 600, "gutToLeft");
    expect(frame.w === 800 || frame.h === 600).toBe(true);
    expect(frame.x + frame.w).toBeLessThanOrEqual(800 + 1e-6);
    expect(frame.y + frame.h).toBeLessThanOrEqual(600 + 1e-6);
  });

  it("panelToFrameUv round-trips for each gut orientation", () => {
    const pw = 1404;
    const ph = 1872;
    for (const o of TABLET_ORIENTATIONS) {
      const samples = [
        { x: 100, y: 200 },
        { x: 700, y: 900 },
        { x: 1300, y: 1800 },
      ];
      for (const s of samples) {
        const { u, v } = panelToFrameUv(s.x, s.y, pw, ph, o);
        const back = frameUvToPanel(u, v, pw, ph, o);
        expect(back.x).toBeCloseTo(s.x, 5);
        expect(back.y).toBeCloseTo(s.y, 5);
      }
    }
  });

  it("gutToLeft keeps panel axes (verified vertical)", () => {
    const { u, v } = panelToFrameUv(702, 936, 1404, 1872, "gutToLeft");
    expect(u).toBeCloseTo(0.5);
    expect(v).toBeCloseTo(0.5);
  });
});

describe("SpatialIndex culling", () => {
  it("returns only viewport-overlapping primitives", () => {
    const doc = new InfiniDocument();
    const ink = { stroke: "#000", strokeWidth: 1 };
    doc.setPrimitives([
      makeRect("near", 0, 0, 10, 10, ink),
      makeRect("far", 10000, 10000, 10, 10, ink),
      makeEllipse("mid", 50, 50, 5, 5, ink),
    ]);
    const view = visibleWorldAabb(200, 200, identityViewport());
    const hits = doc.queryVisible(view).map((p) => p.id);
    expect(hits).toContain("near");
    expect(hits).toContain("mid");
    expect(hits).not.toContain("far");
  });

  it("uses quadtree above flat threshold", () => {
    const doc = new InfiniDocument();
    const ink = { stroke: "#000", strokeWidth: 1 };
    const many = Array.from({ length: 300 }, (_, i) =>
      makePath(
        `p${i}`,
        [
          { x: i * 10, y: 0 },
          { x: i * 10 + 5, y: 5 },
        ],
        ink,
      ),
    );
    doc.setPrimitives(many);
    expect(doc.indexMode).toBe("quadtree");
    expect(doc.queryVisible({ minX: 0, minY: -1, maxX: 30, maxY: 10 }).length).toBeGreaterThan(0);
  });
});
