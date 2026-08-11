/**
 * STORY-IN-018 / @SRS-IN-13 — tool intent transport
 */

import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  UndoRing,
  buildPickables,
  applyToolIntent,
  normalizeStrokeIntent,
  messageCarriesToolMode,
  commitStrokeWithEncloseRecognition,
} from "../src/document";
import type { SmartGroupNode } from "../src/document/types";

describe("STORY-IN-018 / SRS-IN-13 tool intent transport", () => {
  it("normalizes stroke intent default-safe", () => {
    expect(normalizeStrokeIntent(undefined)).toBe("ink");
    expect(normalizeStrokeIntent("ink")).toBe("ink");
    expect(normalizeStrokeIntent("enclose")).toBe("enclose");
    expect(normalizeStrokeIntent("weird")).toBe("ink");
  });

  it("doc_snapshot pickables list SmartGroup world bounds", () => {
    const tree = new VectorDocument();
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 40, height: 20 },
        transform: { x: 100, y: 50, rotation: 0, scaleX: 2, scaleY: 2 },
        children: [],
      },
    });
    const pickables = buildPickables(tree);
    expect(pickables).toHaveLength(1);
    expect(pickables[0]).toMatchObject({
      id: "sg_1",
      kind: "smart_group",
    });
    // world AABB: translate + bounds*scale
    expect(pickables[0].bounds.minX).toBeCloseTo(100);
    expect(pickables[0].bounds.minY).toBeCloseTo(50);
    expect(pickables[0].bounds.maxX).toBeCloseTo(180);
    expect(pickables[0].bounds.maxY).toBeCloseTo(90);
  });

  it("tool_intent move applies set_smart_transform without tool mode", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "sg",
      type: "create_smart_group",
      payload: {
        id: "sg_1",
        bounds: { x: 0, y: 0, width: 10, height: 10 },
        transform: { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1 },
        children: [],
      },
    });
    const msg = {
      type: "tool_intent" as const,
      action: "move" as const,
      nodeId: "sg_1",
      delta: { dx: 15, dy: -5 },
      seq: 1,
    };
    expect(messageCarriesToolMode(msg as unknown as Record<string, unknown>)).toBe(false);
    const r = applyToolIntent(tree, undo, msg);
    expect(r.applied).toBe(true);
    const sg = tree.indexById().get("sg_1") as SmartGroupNode;
    expect(sg.transform.x).toBeCloseTo(15);
    expect(sg.transform.y).toBeCloseTo(-5);
  });

  it("tool_intent unknown nodeId is ignored", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    const r = applyToolIntent(tree, undo, {
      type: "tool_intent",
      action: "move",
      nodeId: "missing",
      delta: { dx: 1, dy: 1 },
    });
    expect(r).toMatchObject({ applied: false, reason: "unknown_nodeId" });
  });

  it("enclose intent still reaches recognition path", () => {
    const tree = new VectorDocument();
    const undo = new UndoRing();
    tree.applyOp({
      opId: "ink",
      type: "append_ink",
      payload: {
        id: "c",
        samples: [
          { x: 60, y: 60 },
          { x: 70, y: 70 },
        ],
        style: { stroke: "#000", strokeWidth: 2 },
      },
    });
    const r = commitStrokeWithEncloseRecognition(tree, undo, {
      id: "box",
      intent: normalizeStrokeIntent("enclose"),
      points: [
        { x: 40, y: 40 },
        { x: 160, y: 40 },
        { x: 160, y: 160 },
        { x: 40, y: 160 },
        { x: 40, y: 40 },
      ],
    });
    expect(r.kind).toBe("created");
  });

  it("viewport and doc_snapshot shapes never carry tool mode", () => {
    expect(
      messageCarriesToolMode({
        type: "viewport",
        translate: { x: 0, y: 0 },
        scale: 1,
      }),
    ).toBe(false);
    expect(
      messageCarriesToolMode({
        type: "doc_snapshot",
        nodes: [],
        pickables: [],
      }),
    ).toBe(false);
    expect(messageCarriesToolMode({ type: "tool_mode", tool: "pen" })).toBe(true);
  });
});
