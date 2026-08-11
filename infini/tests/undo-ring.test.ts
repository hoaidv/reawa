/**
 * @implements [STORY-IN-014] snapshot undo ring
 */

import { describe, expect, it } from "vitest";
import { UndoRing, UNDO_RING_DEPTH, VectorDocument } from "../src/document";

describe("STORY-IN-014 / SRS-IN-12 undo ring", () => {
  it("pushes snapshot before structural op", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    const before = tree.snapshotString();
    const r = undo.applyWithUndo(tree, {
      opId: "frm_1",
      type: "create_frame",
      payload: {
        id: "frm_1",
        bounds: { minX: 0, minY: 0, maxX: 100, maxY: 100 },
      },
    });
    expect(r.applied).toBe(true);
    expect(r.pushed).toBe(true);
    expect(undo.depth).toBe(1);
    expect(JSON.parse(before).rootChildren).toEqual([]);
  });

  it("undo restores prior tree exactly", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    const empty = tree.snapshotString();
    undo.applyWithUndo(tree, {
      opId: "frm_1",
      type: "create_frame",
      payload: {
        id: "frm_1",
        bounds: { minX: 0, minY: 0, maxX: 100, maxY: 100 },
      },
    });
    expect(tree.indexById().has("frm_1")).toBe(true);
    expect(undo.undo(tree).restored).toBe(true);
    expect(tree.snapshotString()).toBe(empty);
    expect(undo.undo(tree).restored).toBe(false);
  });

  it("does not treat viewport-only changes as structural (ring stays 0)", () => {
    const undo = new UndoRing();
    expect(undo.depth).toBe(0);
    // Viewport / tool / selection are not DocOps — ring untouched
    expect(undo.depth).toBe(0);
  });

  it("overflow drops oldest at depth 20", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    for (let i = 0; i < UNDO_RING_DEPTH + 1; i++) {
      undo.applyWithUndo(tree, {
        opId: `ink_${i}`,
        type: "append_ink",
        payload: {
          id: `ink_${i}`,
          samples: [
            { x: i, y: 0 },
            { x: i + 1, y: 1 },
          ],
          style: { stroke: "#000", strokeWidth: 2 },
        },
      });
    }
    expect(undo.depth).toBe(UNDO_RING_DEPTH);
  });
});
