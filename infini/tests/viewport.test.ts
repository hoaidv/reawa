import { describe, expect, it } from "vitest";
import {
  identityViewport,
  panByScreenDelta,
  preserveCenterOnResize,
  screenToWorld,
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
