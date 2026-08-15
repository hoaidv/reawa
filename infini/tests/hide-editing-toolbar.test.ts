/**
 * STORY-IN-031 / @SRS-IN-14 — Infini desktop has 0 authoring chrome.
 * Maps hide-editing-toolbar.feature (REQ-04 deprecated; leftover ToolStrip gone).
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { panByScreenDelta, zoomAtScreenPoint } from "../src/canvas/Viewport";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

function src(rel: string): string {
  return readFileSync(join(root, rel), "utf8");
}

describe("SRS-IN-14 hide editing toolbar (STORY-IN-031)", () => {
  it("region ToolStrip is absent from the running canvas host", () => {
    const stage = src("src/canvas/CanvasStage.tsx");
    const app = src("src/App.tsx");
    const css = src("src/styles/app.css");
    expect(stage).not.toMatch(/data-region=["']ToolStrip["']/);
    expect(app).not.toMatch(/data-region=["']ToolStrip["']/);
    expect(stage).not.toContain("from \"./ToolStrip\"");
    expect(css).not.toContain("c-tool-strip");
  });

  it("region SelectionOverlay and transform handles are absent", () => {
    const stage = src("src/canvas/CanvasStage.tsx");
    const css = src("src/styles/app.css");
    expect(stage).not.toMatch(/data-region=["']SelectionOverlay["']/);
    expect(stage).not.toMatch(/data-handle=/);
    expect(css).not.toContain("c-selection-overlay");
    expect(css).not.toContain("c-handle-");
  });

  it("pan zoom chrome still works", () => {
    const stage = src("src/canvas/CanvasStage.tsx");
    expect(stage).toContain("data-region=\"StatusZoom\"");
    expect(stage).toContain("onWheel");
    expect(stage).toContain("panByScreenDelta");
    expect(stage).toContain("zoomAtScreenPoint");
    const vp = panByScreenDelta({ translate: { x: 0, y: 0 }, scale: 1 }, 10, -4);
    expect(vp.translate).toEqual({ x: 10, y: -4 });
    const z = zoomAtScreenPoint({ translate: { x: 0, y: 0 }, scale: 1 }, { x: 0, y: 0 }, 2);
    expect(z.scale).toBe(2);
  });

  it("does not add a desktop Pen tool", () => {
    const stage = src("src/canvas/CanvasStage.tsx");
    expect(stage).not.toMatch(/data-region=["']ToolStrip["']/);
    expect(stage).not.toMatch(/\bPen\b/);
    expect(stage).not.toContain("tool.pen");
  });
});
