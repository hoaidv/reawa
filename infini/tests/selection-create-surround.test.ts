/**
 * STORY-IN-017 / @SRS-IN-16 — selection create surround
 */

import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  UndoRing,
  commitLiveStrokeToTree,
  createSmartGroupFromSelection,
  qualifiesAsSurround,
  closedPathForTest,
  pointInPolygonEvenOdd,
} from "../src/document";
import type { SmartGroupNode } from "../src/document/types";

function box(id: string, x: number, y: number, w: number, h: number) {
  return {
    id,
    points: [
      { x, y },
      { x: x + w, y },
      { x: x + w, y: y + h },
      { x, y: y + h },
      { x, y },
    ],
  };
}

describe("STORY-IN-017 / SRS-IN-16 selection create surround", () => {
  it("surround winner becomes boundary; others content with UV", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    commitLiveStrokeToTree(tree, box("outer", 0, 0, 100, 100));
    commitLiveStrokeToTree(tree, {
      id: "inner",
      points: [
        { x: 40, y: 40 },
        { x: 50, y: 45 },
        { x: 45, y: 55 },
      ],
    });

    const r = createSmartGroupFromSelection(tree, undo, ["outer", "inner"]);
    expect(r.kind).toBe("created");
    if (r.kind !== "created") return;
    const sg = tree.indexById().get(r.smartGroupId) as SmartGroupNode;
    expect(sg.children.find((c) => c.id === "outer")?.role).toBe("boundary");
    const content = sg.children.find((c) => c.id === "inner")!;
    expect(content.role).toBe("content");
    expect(content.layoutOffset).toBeDefined();
    expect(sg.bounds.width).toBeCloseTo(100);
    expect(sg.bounds.height).toBeCloseTo(100);
  });

  it("refuses when no surround qualifies", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    commitLiveStrokeToTree(tree, {
      id: "a",
      points: [
        { x: 0, y: 0 },
        { x: 10, y: 0 },
      ],
    });
    commitLiveStrokeToTree(tree, {
      id: "b",
      points: [
        { x: 50, y: 50 },
        { x: 60, y: 50 },
      ],
    });
    const before = tree.snapshotString();
    const r = createSmartGroupFromSelection(tree, undo, ["a", "b"]);
    expect(r).toMatchObject({ kind: "refused", reason: "no_surround" });
    expect(tree.snapshotString()).toBe(before);
  });

  it("later sibling wins among qualifying surrounds", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    // earlyBox sits inside lateBox but does not surround inner.
    // lateBox surrounds both earlyBox + inner → only late qualifies (and is later sibling).
    commitLiveStrokeToTree(tree, box("earlyBox", 10, 10, 15, 15));
    commitLiveStrokeToTree(tree, box("lateBox", 0, 0, 100, 100));
    commitLiveStrokeToTree(tree, {
      id: "inner",
      points: [
        { x: 40, y: 40 },
        { x: 50, y: 50 },
      ],
    });
    const r = createSmartGroupFromSelection(tree, undo, [
      "earlyBox",
      "lateBox",
      "inner",
    ]);
    expect(r.kind).toBe("created");
    if (r.kind !== "created") return;
    expect(r.boundaryId).toBe("lateBox");
  });

  it("undo restores prior snapshot", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    commitLiveStrokeToTree(tree, box("outer", 0, 0, 80, 80));
    commitLiveStrokeToTree(tree, {
      id: "inner",
      points: [
        { x: 20, y: 20 },
        { x: 30, y: 30 },
      ],
    });
    const before = tree.snapshotString();
    expect(createSmartGroupFromSelection(tree, undo, ["outer", "inner"]).kind).toBe(
      "created",
    );
    expect(undo.undo(tree).restored).toBe(true);
    expect(tree.snapshotString()).toBe(before);
  });

  it("even-odd PIP + artificial close helpers", () => {
    const open = [
      { x: 0, y: 0 },
      { x: 10, y: 0 },
      { x: 10, y: 10 },
      { x: 0, y: 10 },
    ];
    const closed = closedPathForTest(open);
    expect(closed[0]).toEqual(closed[closed.length - 1]);
    expect(pointInPolygonEvenOdd(5, 5, closed)).toBe(true);
    expect(pointInPolygonEvenOdd(50, 50, closed)).toBe(false);

    const outer = {
      id: "o",
      kind: "ink" as const,
      samples: closed,
      style: { stroke: "#000", strokeWidth: 1 },
    };
    const inner = {
      id: "i",
      kind: "ink" as const,
      samples: [
        { x: 3, y: 3 },
        { x: 4, y: 4 },
      ],
      style: { stroke: "#000", strokeWidth: 1 },
    };
    expect(qualifiesAsSurround(outer, [inner])).toBe(true);
  });
});
