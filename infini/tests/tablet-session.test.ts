/**
 * STORY-IN-009 / @SRS-IN-07 (+ @SRS-IN-08) — Infini tablet session.
 */
import { describe, expect, it } from "vitest";
import { InfiniDocument } from "../src/canvas/Document";
import { identityViewport, strokeCssWidthFromWorld } from "../src/canvas/Viewport";
import { VectorDocument } from "../src/document";
import {
  MemoryTransport,
  TabletSession,
  type DocOpMessage,
} from "../src/session";

const style = { stroke: "#1C2430", strokeWidth: 2 };

function liveSession() {
  const tree = new VectorDocument();
  const world = new InfiniDocument();
  const transport = new MemoryTransport();
  const logs: Array<{ msg: string; detail?: unknown }> = [];
  const session = new TabletSession({
    tree,
    world,
    transport,
    cssWidth: 800,
    cssHeight: 600,
    log: (msg, detail) => logs.push({ msg, detail }),
  });
  session.connect();
  return { tree, world, transport, session, logs };
}

describe("SRS-IN-07 pan zoom emits viewport", () => {
  it("emits viewport with translate scale drawingRegion seq", () => {
    const { session, transport } = liveSession();
    const vp = { translate: { x: -120, y: 40 }, scale: 1.5 };
    const msg = session.publishViewport(vp);
    expect(msg).not.toBeNull();
    expect(transport.viewports).toHaveLength(1);
    const out = transport.viewports[0];
    expect(out.type).toBe("viewport");
    expect(out.translate).toEqual({ x: -120, y: 40 });
    expect(out.scale).toBe(1.5);
    expect(out.drawingRegion.minX).toBeLessThan(out.drawingRegion.maxX);
    expect(out.seq).toBe(1);

    // Force next emit past coalesce window
    session.publishViewport(vp, { force: true });
    expect(transport.viewports[1].seq).toBe(2);
  });
});

describe("SRS-IN-07 tablet frame drawingRegion and coalesce", () => {
  it("drawingRegion is tablet frame AABB inside full window", () => {
    const { session } = liveSession();
    const vp = identityViewport();
    const msg = session.publishViewport(vp, { force: true })!;
    const full = session.fullWindowWorldAabb(vp);
    const region = msg.drawingRegion;
    expect(region.minX).toBeGreaterThanOrEqual(full.minX - 1e-9);
    expect(region.maxX).toBeLessThanOrEqual(full.maxX + 1e-9);
    expect(region.minY).toBeGreaterThanOrEqual(full.minY - 1e-9);
    expect(region.maxY).toBeLessThanOrEqual(full.maxY + 1e-9);
    // Frame does not fill host (10% margin) → strictly inside
    expect(region.maxX - region.minX).toBeLessThan(full.maxX - full.minX);
  });

  it("coalesces to ≤30 Hz and flushes settle pose", () => {
    let t = 0;
    const tree = new VectorDocument();
    const transport = new MemoryTransport();
    const session = new TabletSession({
      tree,
      transport,
      cssWidth: 800,
      cssHeight: 600,
      nowMs: () => t,
    });
    session.connect();

    for (let i = 0; i < 60; i++) {
      t = i * (1000 / 60); // ~60 updates in 1s
      session.publishViewport({
        translate: { x: -i, y: i },
        scale: 1 + i * 0.001,
      });
    }
    expect(transport.viewports.length).toBeLessThanOrEqual(30);
    expect(transport.viewports.length).toBeGreaterThan(0);

    t += 50;
    const settle = { translate: { x: -999, y: 42 }, scale: 2 };
    const flushed = session.flushViewport(settle);
    expect(flushed).not.toBeNull();
    expect(flushed!.translate).toEqual({ x: -999, y: 42 });
    expect(flushed!.scale).toBe(2);
    expect(transport.viewports.at(-1)!.translate).toEqual({ x: -999, y: 42 });
  });
});

describe("SRS-IN-08 world stroke width scales with viewport", () => {
  it("CSS line width halves when scale halves", () => {
    expect(strokeCssWidthFromWorld(2, 1)).toBe(2);
    expect(strokeCssWidthFromWorld(2, 0.5)).toBe(1);
  });
});

describe("SRS-IN-07 append_ink updates tree and WorldLayer", () => {
  it("applies idempotently and syncs world", () => {
    const { tree, world, session } = liveSession();
    tree.applyOp({
      opId: "frm",
      type: "create_frame",
      payload: {
        id: "frm_1",
        bounds: { minX: 0, minY: 0, maxX: 400, maxY: 300 },
      },
    });

    const wire: DocOpMessage = {
      type: "doc_op",
      opId: "ink_1",
      opType: "append_ink",
      source: "epaper",
      payload: {
        id: "ink_node",
        parentId: "frm_1",
        samples: [
          { x: 10, y: 20 },
          { x: 12, y: 24 },
        ],
        style,
      },
    };

    expect(session.receiveDocOp(wire).applied).toBe(true);
    expect(tree.indexById().has("ink_node")).toBe(true);
    expect(world.all().some((p) => p.id === "ink_node")).toBe(true);

    const snap = tree.snapshotString();
    expect(session.receiveDocOp(wire).reason).toBe("duplicate_opId");
    expect(tree.snapshotString()).toBe(snap);
  });
});

describe("SRS-IN-07 structure emit vs in-flight stroke", () => {
  it("queues while stroke in flight then emits", () => {
    const { session, transport, tree } = liveSession();
    session.setEpaperStrokeInFlight(true);

    const r = session.emitLocalStructureOp({
      opId: "struct_1",
      type: "create_primitive",
      source: "infini",
      payload: {
        id: "rect_1",
        geom: { kind: "rect", x: 0, y: 0, w: 10, h: 10 },
        style,
      },
    });
    expect(r.queued).toBe(true);
    expect(r.emitted).toBe(false);
    expect(transport.docOps).toHaveLength(0);
    expect(tree.indexById().has("rect_1")).toBe(true);
    expect(session.queuedStructureCount).toBe(1);

    session.setEpaperStrokeInFlight(false);
    expect(transport.docOps).toHaveLength(1);
    expect(transport.docOps[0].opType).toBe("create_primitive");
    expect(transport.docOps[0].source).toBe("infini");

    // No stroke — emit immediately
    const r2 = session.emitLocalStructureOp({
      opId: "struct_2",
      type: "create_group",
      source: "infini",
      payload: { id: "grp_1" },
    });
    expect(r2.emitted).toBe(true);
    expect(transport.docOps).toHaveLength(2);
  });
});

describe("SRS-IN-07 duplicate and unknown op", () => {
  it("ignores duplicate and logs unknown without crash", () => {
    const { session, tree, logs } = liveSession();
    const op: DocOpMessage = {
      type: "doc_op",
      opId: "once",
      opType: "create_primitive",
      source: "infini",
      payload: {
        id: "p1",
        geom: { kind: "rect", x: 1, y: 2, w: 3, h: 4 },
        style,
      },
    };
    expect(session.receiveDocOp(op).applied).toBe(true);
    const snap = tree.snapshotString();
    expect(session.receiveDocOp(op).reason).toBe("duplicate_opId");
    expect(tree.snapshotString()).toBe(snap);

    const unknown: DocOpMessage = {
      type: "doc_op",
      opId: "weird",
      opType: "teleport_ink",
      source: "epaper",
      payload: {},
    };
    expect(() => session.receiveDocOp(unknown)).not.toThrow();
    expect(session.receiveDocOp(unknown).reason).toMatch(/unknown_type/);
    expect(logs.some((l) => l.msg.includes("unknown"))).toBe(true);
    expect(tree.snapshotString()).toBe(snap);
  });
});

describe("SRS-IN-08 viewport map apply latency", () => {
  it("map apply completes within 100 ms of emit (stub peer)", () => {
    const { session, transport } = liveSession();
    let latency = Infinity;
    transport.onViewportApply = (_msg, appliedAt) => {
      latency = appliedAt - session.lastViewportEmitAtMs;
    };
    session.publishViewport(identityViewport());
    expect(latency).toBeLessThan(100);
    expect(Number.isFinite(latency)).toBe(true);
  });
});
