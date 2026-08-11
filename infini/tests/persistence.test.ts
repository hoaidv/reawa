/**
 * STORY-IN-008 / @SRS-IN-09 — SVG persistence + dual op fixtures.
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import {
  VectorDocument,
  parseInfiniSvg,
  serializeInfiniSvg,
} from "../src/document";

const here = dirname(fileURLToPath(import.meta.url));
const style = { stroke: "#1C2430", strokeWidth: 2 };

function loadFixture(name: string): Record<string, unknown> {
  const p = join(here, "fixtures/ops", name);
  return JSON.parse(readFileSync(p, "utf8")) as Record<string, unknown>;
}

describe("SRS-IN-09 SVG round-trip preserves ink channels", () => {
  it("keeps pressure, tiltX, extras.distance", () => {
    const doc = new VectorDocument();
    doc.applyOp({
      opId: "f",
      type: "create_frame",
      payload: {
        id: "frm_1",
        bounds: { minX: 0, minY: 0, maxX: 800, maxY: 600 },
      },
    });
    doc.applyOp({
      opId: "ink",
      type: "append_ink",
      payload: {
        id: "ink_1",
        parentId: "frm_1",
        samples: [
          { x: 10, y: 20, pressure: 0.42, tiltX: 0.1, t: 0 },
          {
            x: 12,
            y: 24,
            pressure: 0.55,
            tiltX: 0.12,
            t: 16,
            extras: { distance: 0 },
          },
        ],
        style,
      },
    });

    const svg = serializeInfiniSvg(doc);
    const loaded = parseInfiniSvg(svg);
    expect(loaded.ok).toBe(true);
    if (!loaded.ok) return;

    const ink = loaded.doc.indexById().get("ink_1");
    expect(ink?.kind).toBe("ink");
    if (ink?.kind !== "ink") return;
    expect(ink.samples[0].x).toBe(10);
    expect(ink.samples[0].y).toBe(20);
    expect(ink.samples[0].pressure).toBe(0.42);
    expect(ink.samples[0].tiltX).toBe(0.1);
    expect(ink.samples[1].extras?.distance).toBe(0);
  });
});

describe("SRS-IN-09 fail closed vs foreign fluff", () => {
  it("fails closed when Infini-required samples missing", () => {
    const prior = new VectorDocument();
    prior.applyOp({
      opId: "keep",
      type: "create_primitive",
      payload: {
        id: "keep_me",
        geom: { kind: "rect", x: 1, y: 2, w: 3, h: 4 },
        style,
      },
    });
    const snap = prior.snapshotString();

    const bad = `<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg" data-infini-doc-version="1">
  <polyline data-infini-kind="ink" data-infini-id="ink_bad" points="0,0 1,1" stroke="#000" stroke-width="1" fill="none" />
</svg>`;
    const result = parseInfiniSvg(bad);
    expect(result.ok).toBe(false);
    if (result.ok) return;
    expect(result.error).toMatch(/samples/i);

    // prior unchanged when caller does not replace on failure
    expect(prior.snapshotString()).toBe(snap);
  });

  it("skips foreign fluff and still loads Infini content", () => {
    const svg = `<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg" data-infini-doc-version="1">
  <circle cx="5" cy="5" r="2" fill="red" />
  <rect data-infini-kind="primitive" data-infini-id="rect_1" x="0" y="0" width="10" height="10" stroke="#1C2430" stroke-width="2" fill="none" />
</svg>`;
    const result = parseInfiniSvg(svg);
    expect(result.ok).toBe(true);
    if (!result.ok) return;
    expect(result.warnings.some((w) => w.includes("foreign"))).toBe(true);
    expect(result.doc.indexById().has("rect_1")).toBe(true);
  });
});

describe("SRS-IN-09 dual TS and Qt op fixtures", () => {
  it("shares append_ink and structure op envelopes", () => {
    const append = loadFixture("append_ink.json");
    const structure = loadFixture("create_primitive.json");

    for (const op of [append, structure]) {
      expect(op).toHaveProperty("opId");
      expect(op).toHaveProperty("type");
      expect(op).toHaveProperty("payload");
      expect(op).toHaveProperty("source");
      expect(["epaper", "infini"]).toContain(op.source);
    }

    expect(append.type).toBe("append_ink");
    expect(structure.type).toBe("create_primitive");

    // Byte-identical to Epaper Qt fixture path
    const epaperAppend = readFileSync(
      join(here, "../../epaper/tests/fixtures/ops/append_ink.json"),
      "utf8",
    );
    const infiniAppend = readFileSync(
      join(here, "fixtures/ops/append_ink.json"),
      "utf8",
    );
    expect(infiniAppend).toBe(epaperAppend);

    const epaperStruct = readFileSync(
      join(here, "../../epaper/tests/fixtures/ops/create_primitive.json"),
      "utf8",
    );
    const infiniStruct = readFileSync(
      join(here, "fixtures/ops/create_primitive.json"),
      "utf8",
    );
    expect(infiniStruct).toBe(epaperStruct);
  });
});
