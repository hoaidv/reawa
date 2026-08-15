/**
 * STORY-IN-010 / @SRS-IN-10 — tool-armed enclose recognition
 */

import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  UndoRing,
  commitStrokeWithEncloseRecognition,
  commitLiveStrokeToTree,
  MIN_ENCLOSE_WORLD,
  fractionSamplesInside,
  flattenDrawables,
  smartGroupWorldAabb,
} from "../src/document";
import type { InkNode, SmartGroupNode } from "../src/document/types";

const style = { stroke: "#000", strokeWidth: 2 };

function rectPoints(x: number, y: number, w: number, h: number) {
  return [
    { x, y },
    { x: x + w, y },
    { x: x + w, y: y + h },
    { x, y: y + h },
    { x, y },
  ];
}

describe("STORY-IN-010 / SRS-IN-10 enclose recognition", () => {
  it("creates Smart Group immediately on successful enclose", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    commitLiveStrokeToTree(tree, {
      id: "ink_in",
      points: [
        { x: 60, y: 60 },
        { x: 80, y: 70 },
        { x: 70, y: 90 },
      ],
    });

    const r = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "enclose_1",
      intent: "enclose",
      points: rectPoints(40, 40, 120, 120),
    });
    expect(r.kind).toBe("created");
    if (r.kind !== "created") return;

    const sg = tree.indexById().get(r.smartGroupId) as SmartGroupNode;
    expect(sg.kind).toBe("smart_group");
    expect(sg.bounds.width).toBeGreaterThanOrEqual(MIN_ENCLOSE_WORLD);
    expect(sg.bounds.height).toBeGreaterThanOrEqual(MIN_ENCLOSE_WORLD);

    const boundary = sg.children.find((c) => c.role === "boundary");
    const content = sg.children.filter((c) => c.role === "content");
    expect(boundary?.id).toBe("enclose_1");
    expect(content).toHaveLength(1);
    expect(content[0].id).toBe("ink_in");
    expect(content[0].layoutOffset).toBeDefined();
    expect(content[0].layoutOffset!.u).toBeGreaterThan(0);
    expect(content[0].layoutOffset!.u).toBeLessThan(1);
    // Local-space: transform carries world origin; bounds at (0,0).
    expect(sg.transform.x).toBeCloseTo(40);
    expect(sg.transform.y).toBeCloseTo(40);
    expect(sg.bounds.x).toBe(0);
    expect(sg.bounds.y).toBe(0);
    // Flattened content stays near original world location (not vanished).
    const box = smartGroupWorldAabb(sg);
    expect(box.minX).toBeCloseTo(40);
    expect(box.minY).toBeCloseTo(40);
    const drawables = flattenDrawables(tree);
    const ink = drawables.find((d) => d.id === "ink_in");
    expect(ink?.kind).toBe("ink");
    if (ink?.kind === "ink") {
      expect(ink.samples[0].x).toBeGreaterThan(50);
      expect(ink.samples[0].x).toBeLessThan(90);
    }

    // Enclose stroke not also at root as ordinary ink
    expect(tree.rootChildren.some((n) => n.id === "enclose_1")).toBe(false);
    expect(tree.rootChildren.some((n) => n.id === "ink_in")).toBe(false);
  });

  it("skips recognition for non-enclose intent", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    const before = tree.rootChildren.length;
    const r = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "pen_1",
      intent: "ink",
      points: rectPoints(0, 0, 100, 100),
    });
    expect(r.kind).toBe("ordinary_ink");
    expect(r).toMatchObject({ reason: "not_enclose_intent" });
    expect(tree.rootChildren.length).toBe(before + 1);
    expect(tree.indexById().get("pen_1")?.kind).toBe("ink");
    expect([...tree.indexById().values()].some((n) => n.kind === "smart_group")).toBe(
      false,
    );
  });

  it("failed guards leave ordinary ink (too small)", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();

    const small = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "tiny",
      intent: "enclose",
      points: rectPoints(0, 0, 20, 20),
    });
    expect(small.kind).toBe("ordinary_ink");
    expect(small).toMatchObject({ reason: "too_small" });
    expect(tree.indexById().get("tiny")?.kind).toBe("ink");
  });

  it("empty closed enclose creates a boundary-only box", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    const empty = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "empty_box",
      intent: "enclose",
      points: rectPoints(200, 200, 100, 100),
    });
    expect(empty.kind).toBe("created");
    expect(
      [...tree.indexById().values()].filter((n) => n.kind === "smart_group"),
    ).toHaveLength(1);
  });

  it("skips ink already inside a Smart Group; captures remaining", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "sg0",
      type: "create_smart_group",
      payload: {
        id: "sg_old",
        bounds: { x: 0, y: 0, width: 50, height: 50 },
        children: [
          {
            id: "already",
            kind: "ink",
            role: "content",
            samples: [
              { x: 10, y: 10 },
              { x: 20, y: 20 },
            ],
            style,
          } satisfies InkNode,
        ],
      },
    });
    commitLiveStrokeToTree(tree, {
      id: "free",
      points: [
        { x: 70, y: 70 },
        { x: 80, y: 80 },
      ],
    });

    const r = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "enclose_mix",
      intent: "enclose",
      points: rectPoints(0, 0, 120, 120),
    });
    expect(r.kind).toBe("created");
    if (r.kind !== "created") return;
    const sg = tree.indexById().get(r.smartGroupId) as SmartGroupNode;
    expect(sg.children.map((c) => c.id).sort()).toEqual(
      ["enclose_mix", "free"].sort(),
    );
    expect(tree.indexById().has("sg_old")).toBe(true);
    expect(tree.indexById().has("already")).toBe(true);
  });

  it("undo restores pre-create snapshot", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    commitLiveStrokeToTree(tree, {
      id: "ink_in",
      points: [
        { x: 55, y: 55 },
        { x: 65, y: 65 },
      ],
    });
    const before = tree.snapshotString();

    const r = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "enclose_u",
      intent: "enclose",
      points: rectPoints(40, 40, 80, 80),
    });
    expect(r.kind).toBe("created");
    expect(undo.depth).toBe(1);

    const undid = undo.undo(tree);
    expect(undid.restored).toBe(true);
    expect(tree.snapshotString()).toBe(before);
  });

  it("fractionSamplesInside is deterministic", () => {
    const bounds = { x: 0, y: 0, width: 10, height: 10 };
    expect(
      fractionSamplesInside(
        [
          { x: 1, y: 1 },
          { x: 2, y: 2 },
          { x: 20, y: 20 },
          { x: 3, y: 3 },
        ],
        bounds,
      ),
    ).toBeCloseTo(0.75);
  });
});
