/**
 * @implements [STORY-IN-012] tree-backed live ink ingestion
 */

import { describe, expect, it } from "vitest";
import { InfiniDocument } from "../src/canvas/Document";
import {
  VectorDocument,
  commitLiveStrokeToTree,
  drawablesToPrimitives,
} from "../src/document";

describe("STORY-IN-012 / SRS-IN-04 tree-backed live ink", () => {
  it("commits stroke samples as Ink in the tree", () => {
    const tree = new VectorDocument();
    const r = commitLiveStrokeToTree(tree, {
      id: "stroke_42",
      points: [
        { x: 10, y: 20 },
        { x: 12, y: 24 },
        { x: 14, y: 22 },
      ],
      width: 2.5,
    });
    expect(r.applied).toBe(true);
    const ink = tree.indexById().get("stroke_42");
    expect(ink?.kind).toBe("ink");
    if (ink?.kind === "ink") {
      expect(ink.samples).toHaveLength(3);
      expect(ink.samples[0]).toMatchObject({ x: 10, y: 20 });
    }
  });

  it("syncFromVectorDoc paints committed ink on WorldLayer", () => {
    const tree = new VectorDocument();
    commitLiveStrokeToTree(tree, {
      id: "ink_node",
      points: [
        { x: 0, y: 0 },
        { x: 5, y: 5 },
      ],
    });
    const world = new InfiniDocument();
    world.syncFromVectorDoc(tree);
    expect(world.all().some((p) => p.id === "ink_node")).toBe(true);
    expect(tree.flatten().some((d) => d.kind === "ink" && d.id === "ink_node")).toBe(
      true,
    );
    expect(drawablesToPrimitives(tree.flatten()).some((p) => p.id === "ink_node")).toBe(
      true,
    );
  });

  it("rebuild from tree keeps committed ink addressable (SoT)", () => {
    const tree = new VectorDocument();
    commitLiveStrokeToTree(tree, {
      id: "keep_me",
      points: [
        { x: 1, y: 1 },
        { x: 2, y: 2 },
      ],
    });
    const world = new InfiniDocument();
    world.syncFromVectorDoc(tree);
    // Simulate rebuild: re-sync from tree (not WorldLayer-only storage)
    world.syncFromVectorDoc(tree);
    expect(tree.indexById().has("keep_me")).toBe(true);
    expect(world.all().some((p) => p.id === "keep_me")).toBe(true);
  });

  it("rejects strokes with fewer than 2 samples", () => {
    const tree = new VectorDocument();
    const r = commitLiveStrokeToTree(tree, {
      id: "short",
      points: [{ x: 0, y: 0 }],
    });
    expect(r.applied).toBe(false);
    expect(tree.indexById().has("short")).toBe(false);
  });
});
