/**
 * STORY-IN-016 / @SRS-IN-15 — draw-into membership
 */

import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  UndoRing,
  commitLiveStrokeToTree,
  commitStrokeWithEncloseRecognition,
  tryDrawIntoMembership,
} from "../src/document";
import type { InkNode, SmartGroupNode } from "../src/document/types";

const style = { stroke: "#000", strokeWidth: 2 };

describe("STORY-IN-016 / SRS-IN-15 draw-into membership", () => {
  it("joins Pen stroke into Smart Group with own UV; bounds unchanged", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    const existingUv = { u: 0.2, v: 0.3 };
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 200, height: 200 },
        transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [
          {
            id: "old",
            kind: "ink",
            role: "content",
            layoutOffset: existingUv,
            samples: [
              { x: 40, y: 60 },
              { x: 50, y: 60 },
            ],
            style,
          } satisfies InkNode,
        ],
      },
    });
    const boundsBefore = { ...(tree.indexById().get("sg_1") as SmartGroupNode).bounds };

    commitLiveStrokeToTree(tree, {
      id: "new_ink",
      points: [
        { x: 100, y: 100 },
        { x: 110, y: 105 },
        { x: 105, y: 120 },
      ],
    });
    const joined = tryDrawIntoMembership(tree, undo, "new_ink");
    expect(joined.kind).toBe("joined");

    const sg = tree.indexById().get("sg_1") as SmartGroupNode;
    expect(sg.bounds).toEqual(boundsBefore);
    const old = sg.children.find((c) => c.id === "old")!;
    const neu = sg.children.find((c) => c.id === "new_ink")!;
    expect(old.layoutOffset).toEqual(existingUv);
    expect(neu.role).toBe("content");
    expect(neu.layoutOffset).toBeDefined();
    expect(neu.layoutOffset).not.toEqual(existingUv);
    expect(tree.rootChildren.some((n) => n.id === "new_ink")).toBe(false);
  });

  it("fixedInk join stores local = world − translate (ignores parent scale)", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        transform: { x: 40, y: 40, rotation: 0, scaleX: 2, scaleY: 2 },
        inkScaleMode: "fixedInk",
        children: [],
      },
    });
    commitLiveStrokeToTree(tree, {
      id: "ink",
      points: [
        { x: 80, y: 90 },
        { x: 90, y: 95 },
      ],
    });
    expect(tryDrawIntoMembership(tree, undo, "ink").kind).toBe("joined");
    const sg = tree.indexById().get("sg_1") as SmartGroupNode;
    const neu = sg.children.find((c) => c.id === "ink")!;
    expect(neu.samples[0].x).toBeCloseTo(40);
    expect(neu.samples[0].y).toBeCloseTo(50);
  });

  it("later sibling wins among overlapping groups", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    for (const id of ["sg_a", "sg_b"] as const) {
      tree.applyOp({
        opId: id,
        type: "create_smart_group",
        payload: {
          id,
          bounds: { x: 0, y: 0, width: 100, height: 100 },
          children: [],
        },
      });
    }
    commitLiveStrokeToTree(tree, {
      id: "ink",
      points: [
        { x: 20, y: 20 },
        { x: 30, y: 30 },
      ],
    });
    const r = tryDrawIntoMembership(tree, undo, "ink");
    expect(r).toMatchObject({ kind: "joined", smartGroupId: "sg_b" });
    expect((tree.indexById().get("sg_a") as SmartGroupNode).children).toHaveLength(0);
    expect((tree.indexById().get("sg_b") as SmartGroupNode).children.map((c) => c.id)).toEqual([
      "ink",
    ]);
  });

  it("no qualifying group leaves ordinary parent", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 50, height: 50 },
        children: [],
      },
    });
    commitLiveStrokeToTree(tree, {
      id: "far",
      points: [
        { x: 400, y: 400 },
        { x: 410, y: 410 },
      ],
    });
    const r = tryDrawIntoMembership(tree, undo, "far");
    expect(r).toMatchObject({ kind: "none", reason: "no_qualifying_group" });
    expect(tree.rootChildren.some((n) => n.id === "far")).toBe(true);
  });

  it("membership undo restores prior snapshot", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        children: [],
      },
    });
    commitLiveStrokeToTree(tree, {
      id: "ink",
      points: [
        { x: 10, y: 10 },
        { x: 20, y: 20 },
      ],
    });
    const before = tree.snapshotString();
    expect(tryDrawIntoMembership(tree, undo, "ink").kind).toBe("joined");
    expect(undo.undo(tree).restored).toBe(true);
    expect(tree.snapshotString()).toBe(before);
  });

  it("ordinary stroke_end path runs membership (not enclose)", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 100, height: 100 },
        children: [],
      },
    });
    commitStrokeWithEncloseRecognition(tree, undo, {
      id: "pen",
      intent: "ink",
      points: [
        { x: 25, y: 25 },
        { x: 35, y: 35 },
      ],
    });
    const sg = tree.indexById().get("sg_1") as SmartGroupNode;
    expect(sg.children.some((c) => c.id === "pen")).toBe(true);
  });
});
