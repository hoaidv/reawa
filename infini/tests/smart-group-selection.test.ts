/**
 * STORY-IN-015 / @SRS-IN-11 — selection, move, resize, fixedInk UV
 */

import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  flattenDrawables,
  smartLocalToWorld,
  inkSamplesCentroid,
  seedLayoutOffset,
  pickSmartGroupAt,
  handleSelectionPointer,
  createSelectionSession,
} from "../src/document";
import { TILE_LOD_SCALE } from "../src/canvas/TileCache";
import type { InkNode, SmartGroupNode } from "../src/document/types";

const style = { stroke: "#000", strokeWidth: 2 };

function aabbSize(samples: { x: number; y: number }[]) {
  let minX = Infinity,
    minY = Infinity,
    maxX = -Infinity,
    maxY = -Infinity;
  for (const s of samples) {
    minX = Math.min(minX, s.x);
    minY = Math.min(minY, s.y);
    maxX = Math.max(maxX, s.x);
    maxY = Math.max(maxY, s.y);
  }
  return { w: maxX - minX, h: maxY - minY };
}

function makeSg(partial: Partial<SmartGroupNode> & { id: string }): SmartGroupNode {
  return {
    kind: "smart_group",
    bounds: { x: 0, y: 0, width: 200, height: 100 },
    transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
    inkScaleMode: "withBounds",
    children: [],
    ...partial,
  };
}

describe("STORY-IN-015 / SRS-IN-11 selection + fixedInk UV", () => {
  it("picks topmost overlapping SmartGroup above LOD", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "a",
      type: "create_smart_group",
      payload: {
        id: "sg_a",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [],
      },
    });
    doc.applyOp({
      opId: "b",
      type: "create_smart_group",
      payload: {
        id: "sg_b",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [],
      },
    });
    const hit = pickSmartGroupAt(doc, { x: 50, y: 50 }, { scale: 1 });
    expect(hit?.id).toBe("sg_b");
  });

  it("disables pick below TILE_LOD_SCALE", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "a",
      type: "create_smart_group",
      payload: {
        id: "sg_a",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [],
      },
    });
    expect(pickSmartGroupAt(doc, { x: 10, y: 10 }, { scale: TILE_LOD_SCALE - 0.01 })).toBeNull();
  });

  it("drag move emits one set_smart_transform and does not pan", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 80, height: 60 },
        transform: { x: 10, y: 20, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [],
      },
    });
    const vp = { scale: 1, translate: { x: 0, y: 0 } };
    const vpTranslateBefore = { ...vp.translate };
    let session = createSelectionSession();
    let commits = 0;

    let r = handleSelectionPointer(doc, session, "down", { x: 40, y: 40 }, vp);
    session = r.session;
    expect(session.selectedId).toBe("sg_1");
    expect(r.consumed).toBe(true);

    r = handleSelectionPointer(
      doc,
      session,
      "move",
      { x: 60, y: 55 },
      vp,
      { x: 40, y: 40 },
    );
    session = r.session;
    expect(r.consumed).toBe(true);
    expect(vp.translate).toEqual(vpTranslateBefore);

    r = handleSelectionPointer(doc, session, "up", { x: 60, y: 55 }, vp);
    session = r.session;
    expect(r.commit?.type).toBe("set_smart_transform");
    if (r.commit) {
      commits++;
      doc.applyOp({
        opId: "move1",
        type: "set_smart_transform",
        payload: {
          id: r.commit.id,
          transform: r.commit.transform,
        },
      });
    }
    expect(commits).toBe(1);
    const sg = doc.indexById().get("sg_1") as SmartGroupNode;
    expect(sg.transform.x).toBeCloseTo(30);
    expect(sg.transform.y).toBeCloseTo(35);
    expect(vp.translate).toEqual(vpTranslateBefore);
  });

  it("fixedInk resize preserves UV and sample size; UV placement not translate-only", () => {
    const samplesA: InkNode["samples"] = [
      { x: 40, y: 40 },
      { x: 60, y: 40 },
      { x: 60, y: 60 },
      { x: 40, y: 60 },
    ];
    const samplesB: InkNode["samples"] = [
      { x: 140, y: 20 },
      { x: 160, y: 20 },
      { x: 160, y: 30 },
      { x: 140, y: 30 },
    ];
    const bounds = { x: 0, y: 0, width: 200, height: 100 };
    const uvA = seedLayoutOffset(samplesA, bounds);
    const uvB = seedLayoutOffset(samplesB, bounds);
    expect(uvA.u).toBeCloseTo(0.25);
    expect(uvB.u).toBeCloseTo(0.75);

    const sg = makeSg({
      id: "sg_fix",
      bounds,
      inkScaleMode: "fixedInk",
      transform: { x: 100, y: 50, rotation: 0, scaleX: 1, scaleY: 1 },
      children: [
        {
          id: "ink_a",
          kind: "ink",
          role: "content",
          layoutOffset: uvA,
          samples: samplesA,
          style,
        },
        {
          id: "ink_b",
          kind: "ink",
          role: "content",
          layoutOffset: uvB,
          samples: samplesB,
          style,
        },
      ],
    });

    const sizeA0 = aabbSize(samplesA);
    const sizeB0 = aabbSize(samplesB);

    // Resize bounds width×2 — UV preserved on nodes
    sg.bounds = { x: 0, y: 0, width: 400, height: 100 };
    expect(sg.children[0].layoutOffset).toEqual(uvA);
    expect(sg.children[1].layoutOffset).toEqual(uvB);

    const cA = inkSamplesCentroid(samplesA);
    const w0 = smartLocalToWorld({ x: samplesA[0].x, y: samplesA[0].y }, sg, "content", uvA, cA);
    // UV target for A: (0 + 0.25*400, 50) = (100, 50) local → + translate (100,50) → (200, 100)
    // sample (40,40) shifted by (100-50, 50-50) = (50,0) → (90,40) + T → (190, 90)
    expect(w0.x).toBeCloseTo(190);
    expect(w0.y).toBeCloseTo(90);

    // Not translate-only (would be 40+100, 40+50 = 140, 90)
    expect(w0.x).not.toBeCloseTo(140);

    const worldSamplesA = samplesA.map((s) =>
      smartLocalToWorld({ x: s.x, y: s.y }, sg, "content", uvA, cA),
    );
    const sizeA1 = aabbSize(worldSamplesA);
    expect(sizeA1.w).toBeCloseTo(sizeA0.w, 0);
    expect(sizeA1.h).toBeCloseTo(sizeA0.h, 0);

    const cB = inkSamplesCentroid(samplesB);
    const worldSamplesB = samplesB.map((s) =>
      smartLocalToWorld({ x: s.x, y: s.y }, sg, "content", uvB, cB),
    );
    const sizeB1 = aabbSize(worldSamplesB);
    expect(sizeB1.w).toBeCloseTo(sizeB0.w, 0);
    expect(sizeB1.h).toBeCloseTo(sizeB0.h, 0);
  });

  it("withBounds scales content; boundary always transforms", () => {
    const content: InkNode = {
      id: "c",
      kind: "ink",
      role: "content",
      samples: [
        { x: 0, y: 0 },
        { x: 10, y: 0 },
      ],
      style,
    };
    const boundary: InkNode = {
      id: "b",
      kind: "ink",
      role: "boundary",
      samples: [
        { x: 0, y: 0 },
        { x: 20, y: 0 },
      ],
      style,
    };
    const sg = makeSg({
      id: "sg_wb",
      inkScaleMode: "withBounds",
      transform: { x: 0, y: 0, rotation: 0, scaleX: 2, scaleY: 2 },
      children: [content, boundary],
    });
    const cw = smartLocalToWorld({ x: 10, y: 0 }, sg, "content");
    const bw = smartLocalToWorld({ x: 20, y: 0 }, sg, "boundary");
    expect(cw.x).toBeCloseTo(20);
    expect(bw.x).toBeCloseTo(40);

    const doc = new VectorDocument();
    doc.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: sg.id,
        bounds: sg.bounds,
        transform: sg.transform,
        inkScaleMode: "withBounds",
        children: [content, boundary],
      },
    });
    const drawables = flattenDrawables(doc);
    const dContent = drawables.find((d) => d.id === "c");
    const dBound = drawables.find((d) => d.id === "b");
    expect(dContent?.kind).toBe("ink");
    expect(dBound?.kind).toBe("ink");
    if (dContent?.kind === "ink" && dBound?.kind === "ink") {
      expect(dContent.samples[1].x).toBeCloseTo(20);
      expect(dBound.samples[1].x).toBeCloseTo(40);
    }
  });

  it("below LOD pan wins — selection pointer not consumed for pick", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [],
      },
    });
    const vp = { scale: TILE_LOD_SCALE - 0.05, translate: { x: 0, y: 0 } };
    let session = createSelectionSession();
    let r = handleSelectionPointer(doc, session, "down", { x: 20, y: 20 }, vp);
    session = r.session;
    expect(r.consumed).toBe(false);
    expect(session.selectedId).toBeNull();
    r = handleSelectionPointer(
      doc,
      session,
      "move",
      { x: 40, y: 30 },
      vp,
      { x: 20, y: 20 },
    );
    expect(r.consumed).toBe(false);
    expect(r.panDelta).toEqual({ x: 20, y: 10 });
  });

  it("set_ink_scale_mode op toggles mode", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 10, height: 10 },
        children: [],
      },
    });
    const r = doc.applyOp({
      opId: "m",
      type: "set_ink_scale_mode",
      payload: { id: "sg_1", inkScaleMode: "fixedInk" },
    });
    expect(r.applied).toBe(true);
    expect((doc.indexById().get("sg_1") as SmartGroupNode).inkScaleMode).toBe("fixedInk");
  });
});
