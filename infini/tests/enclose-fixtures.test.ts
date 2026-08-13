/**
 * STORY-EP-016 / @SRS-EP-14 — shared enclose/ fixtures vs Infini recognizer.
 * Does not change recognizeEnclose.ts; maps stroke.armed → intent.
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  UndoRing,
  commitStrokeWithEncloseRecognition,
  smartGroupWorldAabb,
} from "../src/document";
import type { DocOp } from "../src/document/types";
import type { EncloseStrokeInput, StrokeIntent } from "../src/document/recognizeEnclose";
import type { SmartGroupNode } from "../src/document/types";

const here = dirname(fileURLToPath(import.meta.url));

type Fitted = { x: number; y: number; width: number; height: number };

type EncloseFixture = {
  id: string;
  seedOps: DocOp[];
  stroke: {
    id: string;
    armed: "ink_box" | "pen" | string;
    width?: number;
    points: Array<{ x: number; y: number }>;
  };
  expected: {
    verdict: "created" | "ordinary_ink" | "skipped";
    reason?: string;
    fittedBounds: Fitted;
    capturedIds?: string[];
    skippedIds?: string[];
    boundaryId?: string;
  };
};

const FILES = [
  "successful.json",
  "too_small.json",
  "no_content.json",
  "pen_armed.json",
  "already_grouped.json",
] as const;

function loadNamed(dir: string, name: string): string {
  return readFileSync(join(dir, name), "utf8");
}

function loadFixture(name: string): EncloseFixture {
  return JSON.parse(loadNamed(join(here, "fixtures/enclose"), name)) as EncloseFixture;
}

function aabb(points: Array<{ x: number; y: number }>): Fitted {
  let minX = Infinity,
    minY = Infinity,
    maxX = -Infinity,
    maxY = -Infinity;
  for (const p of points) {
    minX = Math.min(minX, p.x);
    minY = Math.min(minY, p.y);
    maxX = Math.max(maxX, p.x);
    maxY = Math.max(maxY, p.y);
  }
  return { x: minX, y: minY, width: maxX - minX, height: maxY - minY };
}

function armedToIntent(armed: string): StrokeIntent {
  return armed === "ink_box" ? "enclose" : "ink";
}

function runFixture(fix: EncloseFixture) {
  const tree = new VectorDocument();
  const undo = new UndoRing();
  for (const op of fix.seedOps) {
    const r = tree.applyOp(op);
    expect(r.applied, r.reason).toBe(true);
  }
  const input: EncloseStrokeInput = {
    id: fix.stroke.id,
    points: fix.stroke.points,
    width: fix.stroke.width,
    intent: armedToIntent(fix.stroke.armed),
    source: "epaper",
  };
  const result = commitStrokeWithEncloseRecognition(tree, undo, input);
  const fitted = aabb(fix.stroke.points);
  return { tree, result, fitted };
}

function verdictOf(result: { kind: string }): string {
  if (result.kind === "created") return "created";
  if (result.kind === "ordinary_ink") return "ordinary_ink";
  return "skipped";
}

describe("STORY-EP-016 / SRS-EP-14 shared enclose fixtures", () => {
  it("docs, epaper, and infini copies are byte-identical", () => {
    const docsDir = join(
      here,
      "../../.docs/modules/infini/features/vector-document/fixtures/enclose",
    );
    const epaperDir = join(here, "../../epaper/tests/fixtures/enclose");
    const infiniDir = join(here, "fixtures/enclose");
    for (const name of FILES) {
      const docs = loadNamed(docsDir, name);
      expect(loadNamed(epaperDir, name), name).toBe(docs);
      expect(loadNamed(infiniDir, name), name).toBe(docs);
    }
  });

  for (const name of FILES) {
    it(`${name} agrees on verdict and fitted bounds`, () => {
      const fix = loadFixture(name);
      const { tree, result, fitted } = runFixture(fix);
      expect(verdictOf(result)).toBe(fix.expected.verdict);
      expect(fitted.x).toBeCloseTo(fix.expected.fittedBounds.x, 6);
      expect(fitted.y).toBeCloseTo(fix.expected.fittedBounds.y, 6);
      expect(fitted.width).toBeCloseTo(fix.expected.fittedBounds.width, 6);
      expect(fitted.height).toBeCloseTo(fix.expected.fittedBounds.height, 6);
      if (fix.expected.reason) {
        expect(result).toMatchObject({ reason: fix.expected.reason });
      }
      if (result.kind === "created") {
        const sg = tree.indexById().get(result.smartGroupId) as SmartGroupNode;
        const world = smartGroupWorldAabb(sg);
        expect(world.minX).toBeCloseTo(fix.expected.fittedBounds.x, 6);
        expect(world.minY).toBeCloseTo(fix.expected.fittedBounds.y, 6);
        expect(world.maxX - world.minX).toBeCloseTo(fix.expected.fittedBounds.width, 6);
        expect(world.maxY - world.minY).toBeCloseTo(fix.expected.fittedBounds.height, 6);
        if (fix.expected.capturedIds) {
          const contentIds = sg.children
            .filter((c) => c.role === "content")
            .map((c) => c.id)
            .sort();
          expect(contentIds).toEqual([...fix.expected.capturedIds].sort());
        }
        if (fix.expected.skippedIds) {
          for (const id of fix.expected.skippedIds) {
            expect(sg.children.some((c) => c.id === id)).toBe(false);
            expect(tree.indexById().has(id)).toBe(true);
          }
        }
      }
    });
  }
});
