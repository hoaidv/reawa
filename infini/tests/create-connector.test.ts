/**
 * STORY-IN-030 / @SRS-IN-09 — create_connector envelope + derived warp.
 * Host tests for .docs/modules/infini/features/vector-document/bdd/create-connector.feature
 */
import { describe, expect, it } from "vitest";
import { InfiniDocument } from "../src/canvas/Document";
import {
  VectorDocument,
  connectorWirePayload,
  flattenDrawables,
  restShapeReconstruction,
  warpConnector,
} from "../src/document";
import type { Anchor, InkNode, RestShape, Vec2 } from "../src/document";
import type { WarpEnd } from "../src/document/connectorWarp";
import { MemoryTransport, TabletSession, type DocChangeMessage } from "../src/session";

const style = { stroke: "#1C2430", strokeWidth: 2 };

const REST: RestShape = {
  spine: [
    { x: 0, y: 0 },
    { x: 100, y: 10 },
    { x: 200, y: 0 },
    { x: 300, y: 10 },
    { x: 400, y: 0 },
  ],
  offsets: [
    { s: 0, d: 0 },
    { s: 0.25, d: 3 },
    { s: 0.5, d: -2 },
    { s: 0.75, d: 4 },
    { s: 1, d: 0 },
  ],
};

function unit(a: Vec2): Vec2 {
  const l = Math.hypot(a.x, a.y);
  return l > 1e-12 ? { x: a.x / l, y: a.y / l } : { x: 1, y: 0 };
}

function endsFromSpine(spine: Vec2[]): { e0: WarpEnd; e1: WarpEnd } {
  const clip = { minX: 0, minY: 0, maxX: 0, maxY: 0 };
  const n = spine.length;
  const e0: WarpEnd = {
    p: { ...spine[0] },
    f: unit({ x: spine[1].x - spine[0].x, y: spine[1].y - spine[0].y }),
    centre: false,
    clip,
    hasClip: false,
  };
  const e1: WarpEnd = {
    p: { ...spine[n - 1] },
    f: unit({ x: spine[n - 2].x - spine[n - 1].x, y: spine[n - 2].y - spine[n - 1].y }),
    centre: false,
    clip,
    hasClip: false,
  };
  return { e0, e1 };
}

function addSmartGroup(
  doc: VectorDocument,
  id: string,
  x: number,
  y: number,
  w: number,
  h: number,
): void {
  doc.applyOp({
    opId: `sg:${id}`,
    type: "create_smart_group",
    payload: {
      id,
      bounds: { x: 0, y: 0, width: w, height: h },
      transform: { x, y, rotation: 0, scaleX: 1, scaleY: 1 },
      inkScaleMode: "fixedInk",
      children: [
        {
          id: `${id}_b`,
          kind: "ink",
          role: "boundary",
          samples: [
            { x: 0, y: 0 },
            { x: w, y: 0 },
            { x: w, y: h },
            { x: 0, y: h },
            { x: 0, y: 0 },
          ],
          style,
        },
      ],
    },
  });
}

const fromEnv: Anchor = {
  nodeId: "A",
  kind: "edge",
  edge: 1,
  t: 0.5,
  drawnN: 1,
  drawnE: 0,
  drawnBoxX: 1,
  drawnBoxY: 0,
  drawnEdgeLocal: { n: 1, e: 0 },
  drawnBoxLocal: { x: 1, y: 0 },
  local: { x: 80, y: 40 },
  hasLocal: true,
};

const toEnv: Anchor = {
  nodeId: "C",
  kind: "edge",
  edge: 3,
  t: 0.5,
  drawnN: 1,
  drawnE: 0,
  drawnBoxX: -1,
  drawnBoxY: 0,
  drawnEdgeLocal: { n: 1, e: 0 },
  drawnBoxLocal: { x: -1, y: 0 },
  local: { x: 0, y: 40 },
  hasLocal: true,
};

const bodyInk: InkNode = {
  id: "ink_body",
  kind: "ink",
  samples: [
    { x: 80, y: 40 },
    { x: 200, y: 42 },
    { x: 300, y: 40 },
  ],
  style,
};

describe("SRS-IN-09 create_connector envelope round-trips with 0 loss", () => {
  it("stores from/to/warpStyle/body/restShape and does not stream warped samples", () => {
    const doc = new VectorDocument();
    addSmartGroup(doc, "A", 0, 0, 80, 80);
    addSmartGroup(doc, "C", 300, 0, 80, 80);
    const restShape = {
      spine: REST.spine.map((p) => ({ ...p })),
      offsets: REST.offsets.map((o) => ({ ...o })),
    };
    const applied = doc.applyOp({
      opId: "conn1",
      type: "create_connector",
      payload: {
        id: "conn_1",
        from: fromEnv,
        to: toEnv,
        warpStyle: "morph",
        body: [bodyInk],
        restShape,
      },
    });
    expect(applied.applied).toBe(true);
    const conn = doc.indexById().get("conn_1");
    expect(conn?.kind).toBe("connector");
    if (conn?.kind !== "connector") return;
    expect(conn.from).toEqual(fromEnv);
    expect(conn.to).toEqual(toEnv);
    expect(conn.warpStyle).toBe("morph");
    expect(conn.restSpine).toEqual(restShape.spine);
    expect(conn.restOffsets).toEqual(restShape.offsets);
    expect(conn.children).toHaveLength(1);
    expect(conn.children?.[0].id).toBe("ink_body");
    expect(conn.children?.[0].samples).toEqual(bodyInk.samples);
    expect(conn.invalid).toBe(false);

    const wire = connectorWirePayload(conn);
    expect(wire.from).toEqual(fromEnv);
    expect(wire.to).toEqual(toEnv);
    expect(wire.warpStyle).toBe("morph");
    expect(wire.restShape).toEqual(restShape);
    expect(wire.body).toEqual(conn.children);
    expect(wire).not.toHaveProperty("warpedSamples");
    expect(wire).not.toHaveProperty("path");
    expect(JSON.stringify(wire)).not.toMatch(/warpedSamples/);
  });

  it("reparents captureIds into the connector body like device", () => {
    const doc = new VectorDocument();
    addSmartGroup(doc, "A", 0, 0, 80, 80);
    addSmartGroup(doc, "C", 300, 0, 80, 80);
    doc.applyOp({
      opId: "ink",
      type: "append_ink",
      payload: { id: "cap_1", samples: bodyInk.samples, style },
    });
    expect(doc.rootChildren.some((n) => n.id === "cap_1")).toBe(true);
    doc.applyOp({
      opId: "conn",
      type: "create_connector",
      payload: {
        id: "conn_cap",
        from: fromEnv,
        to: toEnv,
        warpStyle: "cubic",
        captureIds: ["cap_1"],
        restShape: REST,
      },
    });
    expect(doc.rootChildren.some((n) => n.id === "cap_1")).toBe(false);
    const conn = doc.indexById().get("conn_cap");
    expect(conn?.kind).toBe("connector");
    if (conn?.kind !== "connector") return;
    expect(conn.children?.map((c) => c.id)).toEqual(["cap_1"]);
    expect(doc.indexById().get("cap_1")?.kind).toBe("ink");
  });
});

describe("SRS-IN-09 shared rest shape yields byte-comparable samples", () => {
  it("Morph at rest is bitwise rest-shape reconstruction; two warps diverge 0", () => {
    const { e0, e1 } = endsFromSpine(REST.spine);
    const rec = restShapeReconstruction(REST);
    const morph = warpConnector(REST, e0, e1, "morph");
    expect(morph.mixM).toBe(0);
    expect(morph.samples.length).toBe(rec.length);
    for (let i = 0; i < rec.length; i++) {
      expect(morph.samples[i].x).toBe(rec[i].x);
      expect(morph.samples[i].y).toBe(rec[i].y);
    }
    const again = warpConnector(REST, e0, e1, "morph");
    let divergent = 0;
    for (let i = 0; i < morph.samples.length; i++) {
      if (again.samples[i].x !== morph.samples[i].x || again.samples[i].y !== morph.samples[i].y) {
        divergent += 1;
      }
    }
    expect(divergent).toBe(0);

    const cubic = warpConnector(REST, e0, e1, "cubic");
    let maxd = 0;
    for (let i = 0; i < morph.samples.length; i++) {
      maxd = Math.max(
        maxd,
        Math.hypot(cubic.samples[i].x - morph.samples[i].x, cubic.samples[i].y - morph.samples[i].y),
      );
    }
    expect(maxd).toBeGreaterThan(0.5);
  });

  it("never re-bakes rest: 200 poses then start is bitwise the first warp", () => {
    const rest: RestShape = {
      spine: REST.spine.map((p) => ({ ...p })),
      offsets: REST.offsets.map((o) => ({ ...o })),
    };
    const { e0, e1 } = endsFromSpine(rest.spine);
    const first = warpConnector(rest, e0, e1, "morph");
    for (let i = 0; i < 200; i++) {
      const a: WarpEnd = { ...e0, p: { x: e0.p.x + 12 * Math.sin(i * 0.31), y: e0.p.y + 9 * Math.cos(i * 0.17) } };
      const b: WarpEnd = { ...e1, p: { x: e1.p.x + 7 * Math.cos(i * 0.23), y: e1.p.y + 11 * Math.sin(i * 0.19) } };
      warpConnector(rest, a, b, "morph");
    }
    const last = warpConnector(rest, e0, e1, "morph");
    expect(rest.spine).toEqual(REST.spine);
    expect(last.samples.length).toBe(first.samples.length);
    for (let i = 0; i < first.samples.length; i++) {
      expect(last.samples[i].x).toBe(first.samples[i].x);
      expect(last.samples[i].y).toBe(first.samples[i].y);
    }
  });
});

describe("SRS-IN-09 set_smart_transform emits 0 connector ops", () => {
  it("re-derives geometry and does not emit create_connector", () => {
    const tree = new VectorDocument();
    const world = new InfiniDocument();
    const transport = new MemoryTransport();
    const session = new TabletSession({
      tree,
      world,
      transport,
      cssWidth: 800,
      cssHeight: 600,
    });
    session.connect();

    addSmartGroup(tree, "A", 0, 0, 80, 80);
    addSmartGroup(tree, "C", 300, 0, 80, 80);
    tree.applyOp({
      opId: "conn1",
      type: "create_connector",
      payload: {
        id: "conn_1",
        from: fromEnv,
        to: toEnv,
        warpStyle: "morph",
        restShape: REST,
      },
    });
    const before = tree.indexById().get("conn_1");
    expect(before?.kind).toBe("connector");
    if (before?.kind !== "connector") return;
    const restBefore = JSON.stringify(before.restSpine);
    const x0 = before.warpedSamples?.[0]?.x ?? before.path?.[0]?.x;
    expect(x0).toEqual(expect.any(Number));

    const change: DocChangeMessage = {
      type: "doc_change",
      seq: 1,
      opId: "xfA",
      baseSeq: 0,
      op: {
        type: "set_smart_transform",
        opId: "xfA",
        source: "epaper",
        payload: {
          id: "A",
          transform: { x: 40, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        },
      },
    };
    const r = session.receiveDocChange(change);
    expect(r.applied).toBe(true);
    expect(transport.docOps).toHaveLength(0);

    const after = tree.indexById().get("conn_1");
    expect(after?.kind).toBe("connector");
    if (after?.kind !== "connector") return;
    expect(JSON.stringify(after.restSpine)).toBe(restBefore);
    const x1 = after.warpedSamples?.[0]?.x ?? after.path?.[0]?.x ?? 0;
    expect(Math.abs(x1 - (x0 as number))).toBeGreaterThan(1);
    expect(after.invalid).toBe(false);
    expect([...tree.indexById().keys()].filter((id) => id.startsWith("conn_")).length).toBe(1);
  });
});

describe("SRS-IN-09 missing bound node uses last live pose", () => {
  it("stays drawn and is not marked invalid (D39)", () => {
    const doc = new VectorDocument();
    addSmartGroup(doc, "A", 0, 0, 80, 80);
    addSmartGroup(doc, "C", 300, 0, 80, 80);
    doc.applyOp({
      opId: "conn1",
      type: "create_connector",
      payload: {
        id: "conn_1",
        from: fromEnv,
        to: toEnv,
        warpStyle: "morph",
        restShape: REST,
      },
    });
    const live = doc.indexById().get("conn_1");
    expect(live?.kind).toBe("connector");
    if (live?.kind !== "connector") return;
    expect(live.fromPose?.valid).toBe(true);
    expect(live.toPose?.valid).toBe(true);
    const nSamp = live.warpedSamples?.length ?? 0;
    expect(nSamp).toBeGreaterThan(1);
    const poseX = live.fromPose!.x;
    const poseY = live.fromPose!.y;

    expect(doc.applyOp({ opId: "rmA", type: "remove_node", payload: { id: "A" } }).applied).toBe(
      true,
    );
    expect(doc.indexById().has("A")).toBe(false);
    const orphan = doc.indexById().get("conn_1");
    expect(orphan?.kind).toBe("connector");
    if (orphan?.kind !== "connector") return;
    expect(orphan.invalid).toBe(false);
    expect(orphan.from.nodeId).toBe("A");
    expect(orphan.fromPose?.valid).toBe(true);
    expect(orphan.fromPose?.x).toBe(poseX);
    expect(orphan.fromPose?.y).toBe(poseY);
    expect(orphan.warpedSamples?.length).toBe(nSamp);

    const drawables = flattenDrawables(doc);
    const painted = drawables.find((d) => d.id === "conn_1");
    expect(painted?.kind).toBe("connector");
    if (painted?.kind !== "connector") return;
    expect(painted.invalid).toBeFalsy();
    expect(painted.path.length).toBe(nSamp);
  });
});
