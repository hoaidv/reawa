/**
 * STORY-IN-007 / @SRS-IN-04 — tree ops, idempotency, anchors, flatten.
 */
import { describe, expect, it } from "vitest";
import { InfiniDocument } from "../src/canvas/Document";
import { visibleWorldAabb, identityViewport } from "../src/canvas/Viewport";
import {
  VectorDocument,
  drawablesToPrimitives,
  flattenDrawables,
  resolveAnchor,
} from "../src/document";

const style = { stroke: "#1C2430", strokeWidth: 2 };

describe("SRS-IN-04 tree invariants", () => {
  it("holds unique ids, frame root-only, group no frame, connector resolve", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "f1",
      type: "create_frame",
      payload: {
        id: "frm_1",
        bounds: { minX: 0, minY: 0, maxX: 400, maxY: 300 },
      },
    });
    doc.applyOp({
      opId: "ink1",
      type: "append_ink",
      payload: {
        id: "ink_1",
        parentId: "frm_1",
        samples: [
          { x: 10, y: 20, pressure: 0.4 },
          { x: 12, y: 24, pressure: 0.5 },
        ],
        style,
      },
    });
    doc.applyOp({
      opId: "txt1",
      type: "create_text",
      payload: {
        id: "txt_1",
        parentId: "frm_1",
        box: { minX: 40, minY: 40, maxX: 120, maxY: 60 },
        runs: [{ text: "hi" }],
        style,
      },
    });
    doc.applyOp({
      opId: "rect1",
      type: "create_primitive",
      payload: {
        id: "rect_1",
        geom: { kind: "rect", x: 0, y: 0, w: 100, h: 40 },
        style,
      },
    });
    doc.applyOp({
      opId: "ell1",
      type: "create_primitive",
      payload: {
        id: "ell_1",
        geom: { kind: "ellipse", cx: 200, cy: 80, rx: 30, ry: 20 },
        style,
      },
    });
    doc.applyOp({
      opId: "grp1",
      type: "create_group",
      payload: { id: "grp_1" },
    });
    doc.applyOp({
      opId: "conn1",
      type: "create_connector",
      payload: {
        id: "conn_1",
        from: { nodeId: "rect_1", port: "east" },
        to: { nodeId: "ell_1", port: "left" },
      },
    });

    const ids = doc.allIds();
    expect(new Set(ids).size).toBe(ids.length);
    expect(doc.framesOnlyAtRoot()).toBe(true);
    expect(doc.groupsExcludeFrame()).toBe(true);

    const conn = doc.indexById().get("conn_1");
    expect(conn?.kind).toBe("connector");
    if (conn?.kind === "connector") {
      expect(conn.invalid).toBe(false);
    }

    // Frame under group rejected
    const nested = doc.applyOp({
      opId: "bad_frame",
      type: "create_frame",
      payload: {
        id: "frm_nested",
        bounds: { minX: 0, minY: 0, maxX: 10, maxY: 10 },
        parentId: "grp_1",
      },
    });
    // create_frame always roots — if someone tries parentId we ignore and still root-only.
    // Explicit: insert frame via create_group path would fail; nesting check:
    expect(doc.framesOnlyAtRoot()).toBe(true);
    void nested;

    const orphanConn = doc.applyOp({
      opId: "conn_bad",
      type: "create_connector",
      payload: {
        id: "conn_bad",
        from: { nodeId: "missing_a", port: "east" },
        to: { nodeId: "missing_b", port: "west" },
      },
    });
    expect(orphanConn.applied).toBe(true);
    const bad = doc.indexById().get("conn_bad");
    expect(bad?.kind === "connector" && bad.invalid).toBe(true);
  });
});

describe("SRS-IN-04 idempotent opId", () => {
  it("second apply leaves tree unchanged", () => {
    const doc = new VectorDocument();
    const op = {
      opId: "op_100",
      type: "create_primitive" as const,
      payload: {
        id: "p1",
        geom: { kind: "rect" as const, x: 1, y: 2, w: 3, h: 4 },
        style,
      },
    };
    expect(doc.applyOp(op).applied).toBe(true);
    const snap = doc.snapshotString();
    expect(doc.applyOp(op).applied).toBe(false);
    expect(doc.snapshotString()).toBe(snap);
  });
});

describe("SRS-IN-04 connector re-resolve", () => {
  it("recomputes east and ellipse angle after translate", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "r",
      type: "create_primitive",
      payload: {
        id: "rect_1",
        geom: { kind: "rect", x: 0, y: 0, w: 100, h: 40 },
        style,
      },
    });
    doc.applyOp({
      opId: "e",
      type: "create_primitive",
      payload: {
        id: "ell_1",
        geom: { kind: "ellipse", cx: 200, cy: 80, rx: 30, ry: 20 },
        style,
      },
    });
    doc.applyOp({
      opId: "c",
      type: "create_connector",
      payload: {
        id: "conn_1",
        from: { nodeId: "rect_1", port: "east" },
        to: { nodeId: "ell_1", boundary: { angle: 0.785 } },
      },
    });

    doc.applyOp({
      opId: "move",
      type: "translate_node",
      payload: { id: "rect_1", dx: 50, dy: 0 },
    });

    const byId = doc.indexById();
    const from = resolveAnchor({ nodeId: "rect_1", port: "east" }, byId)!;
    const to = resolveAnchor(
      { nodeId: "ell_1", boundary: { angle: 0.785 } },
      byId,
    )!;

    // east midpoint of rect at x=50..150 → east x=150, y=20
    expect(from.x).toBeCloseTo(150);
    expect(from.y).toBeCloseTo(20);

    expect(to.x).toBeCloseTo(200 + 30 * Math.cos(0.785));
    expect(to.y).toBeCloseTo(80 + 20 * Math.sin(0.785));

    const conn = byId.get("conn_1");
    expect(conn?.kind).toBe("connector");
    if (conn?.kind === "connector" && conn.path) {
      expect(conn.path[0].x).toBeCloseTo(from.x);
      expect(conn.path[0].y).toBeCloseTo(from.y);
      expect(conn.path[1].x).toBeCloseTo(to.x);
      expect(conn.path[1].y).toBeCloseTo(to.y);
    }
  });
});

describe("SRS-IN-04 flattenDrawables + WorldLayer", () => {
  it("projects ink and SmartGroup transform; cull/paint via InfiniDocument", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "f",
      type: "create_frame",
      payload: {
        id: "frm_1",
        bounds: { minX: 0, minY: 0, maxX: 800, maxY: 600 },
      },
    });
    doc.applyOp({
      opId: "ink",
      type: "append_ink",
      payload: {
        id: "ink_1",
        parentId: "frm_1",
        samples: [
          { x: 10, y: 10 },
          { x: 20, y: 15 },
        ],
        style,
      },
    });
    doc.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 40, height: 20 },
        transform: { x: 100, y: 50, rotation: 0, scaleX: 2, scaleY: 2 },
        inkScaleMode: "withBounds",
        children: [
          {
            id: "ink_sg",
            kind: "ink",
            role: "content",
            samples: [
              { x: 1, y: 1 },
              { x: 2, y: 2 },
            ],
            style,
          },
        ],
      },
    });

    const drawables = flattenDrawables(doc);
    expect(drawables.some((d) => d.id === "ink_1" && d.kind === "ink")).toBe(true);
    const sgInk = drawables.find((d) => d.id === "ink_sg");
    expect(sgInk?.kind).toBe("ink");
    if (sgInk?.kind === "ink") {
      // local (1,1) * scale 2 + translate (100,50) = (102, 52)
      expect(sgInk.samples[0].x).toBeCloseTo(102);
      expect(sgInk.samples[0].y).toBeCloseTo(52);
    }

    const host = new InfiniDocument();
    host.setPrimitives(drawablesToPrimitives(drawables));
    const view = visibleWorldAabb(400, 300, identityViewport());
    const hits = host.queryVisible(view);
    expect(hits.length).toBeGreaterThan(0);
    expect(() => host.queryVisible(view)).not.toThrow();
  });
});

describe("SRS-IN-07 reparent remove restore_snapshot", () => {
  it("reparents ink under a group and removes it", () => {
    const doc = new VectorDocument();
    doc.applyOp({ opId: "g", type: "create_group", payload: { id: "grp_1" } });
    doc.applyOp({
      opId: "ink",
      type: "append_ink",
      payload: {
        id: "ink_1",
        samples: [
          { x: 0, y: 0 },
          { x: 1, y: 1 },
        ],
        style,
      },
    });
    expect(
      doc.applyOp({
        opId: "rp",
        type: "reparent",
        payload: { id: "ink_1", newParentId: "grp_1", index: 0 },
      }).applied,
    ).toBe(true);
    const grp = doc.indexById().get("grp_1");
    expect(grp?.kind).toBe("group");
    if (grp?.kind === "group") expect(grp.children[0]?.id).toBe("ink_1");

    expect(doc.applyOp({ opId: "rm", type: "remove", payload: { id: "ink_1" } }).applied).toBe(
      true,
    );
    expect(doc.indexById().has("ink_1")).toBe(false);
  });

  it("restore_snapshot replaces the tree wholesale", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "old",
      type: "create_primitive",
      payload: { id: "p1", geom: { kind: "rect", x: 0, y: 0, w: 1, h: 1 }, style },
    });
    expect(
      doc.applyOp({
        opId: "rst",
        type: "restore_snapshot",
        payload: {
          document: {
            version: 1,
            rootChildren: [
              {
                id: "fresh",
                kind: "ink",
                samples: [
                  { x: 2, y: 2 },
                  { x: 3, y: 3 },
                ],
                style,
              },
            ],
          },
        },
      }).applied,
    ).toBe(true);
    expect(doc.indexById().has("fresh")).toBe(true);
    expect(doc.indexById().has("p1")).toBe(false);
  });
});
