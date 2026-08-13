/**
 * STORY-IN-029 / @SRS-IN-17 @SRS-IN-18 @SRS-IN-19 — Device Log decode, ring, filter.
 */
import { createRequire } from "node:module";
import { describe, expect, it } from "vitest";
import { VectorDocument } from "../src/document";
import { filterDebugLog, type DebugLogRecord } from "../src/debuglog/filterDebugLog";

const require = createRequire(import.meta.url);
const {
  DEBUG_RING_CAP,
  decodeDebugLine,
  isDocumentTypeOnDebugPort,
  pushDebugRing,
} = require("../electron/debugLogChannel.cjs") as {
  DEBUG_RING_CAP: number;
  decodeDebugLine: (line: string) =>
    | { kind: "log"; record: DebugLogRecord }
    | { kind: "drop"; reason: string; type?: string };
  isDocumentTypeOnDebugPort: (type: string) => boolean;
  pushDebugRing: (ring: DebugLogRecord[], rec: DebugLogRecord, cap?: number) => number;
};

function logLine(msg: string, extra: Record<string, unknown> = {}): string {
  return JSON.stringify({
    type: "debug_log",
    ts: 1,
    level: "info",
    logger: "qt",
    msg,
    dropped: 0,
    ...extra,
  });
}

describe("SRS-IN-17 debug decoder", () => {
  it("accepts debug_log and never treats it as a document op", () => {
    const doc = new VectorDocument();
    const before = doc.flatten().length;
    const r = decodeDebugLine(logLine("[enclose] armed=ink_box outcome=created"));
    expect(r.kind).toBe("log");
    if (r.kind === "log") expect(r.record.msg).toContain("[enclose]");
    expect(doc.flatten()).toHaveLength(before);
  });

  it("drops viewport, doc_change, and stroke_begin (not forwarded to 9877)", () => {
    for (const type of ["viewport", "doc_change", "stroke_begin"]) {
      const r = decodeDebugLine(JSON.stringify({ type, seq: 1 }));
      expect(r.kind).toBe("drop");
      expect(isDocumentTypeOnDebugPort(type)).toBe(true);
    }
  });

  it("drops bad JSON and keeps going", () => {
    expect(decodeDebugLine("not-json{").kind).toBe("drop");
  });
});

describe("SRS-IN-17 / SRS-IN-19 ring cap 10000", () => {
  it("drops oldest on overflow", () => {
    const ring: DebugLogRecord[] = [];
    for (let i = 0; i < DEBUG_RING_CAP; i++) {
      pushDebugRing(ring, {
        type: "debug_log",
        ts: i,
        level: "info",
        logger: "qt",
        msg: `m${i}`,
        dropped: 0,
      });
    }
    expect(ring).toHaveLength(10000);
    expect(ring[0].msg).toBe("m0");
    pushDebugRing(ring, {
      type: "debug_log",
      ts: 99999,
      level: "info",
      logger: "qt",
      msg: "newest",
      dropped: 0,
    });
    expect(ring).toHaveLength(10000);
    expect(ring[0].msg).toBe("m1");
    expect(ring[ring.length - 1].msg).toBe("newest");
  });
});

describe("SRS-IN-18 filter is view-only", () => {
  const alpha: DebugLogRecord = {
    type: "debug_log",
    ts: 1,
    level: "info",
    logger: "qt",
    msg: "alpha",
    dropped: 0,
  };
  const bravo: DebugLogRecord = {
    type: "debug_log",
    ts: 2,
    level: "warning",
    logger: "qt",
    msg: "bravo",
    dropped: 0,
  };

  it("matches case-insensitively and leaves the buffer unchanged", () => {
    const buffer = [alpha, bravo];
    const shown = filterDebugLog(buffer, "BRA");
    expect(shown).toEqual([bravo]);
    expect(buffer).toEqual([alpha, bravo]);
  });

  it("empty needle restores the full view", () => {
    expect(filterDebugLog([alpha, bravo], "  ")).toEqual([alpha, bravo]);
  });
});

describe("SRS-IN-19 debug_log is not applyOp", () => {
  it("VectorDocument rejects debug_log as unknown_type", () => {
    const doc = new VectorDocument();
    const r = decodeDebugLine(logLine("x"));
    expect(r.kind).toBe("log");
    const result = doc.applyOp({
      opId: "debug_should_not_apply",
      type: "debug_log",
      payload: { msg: "x" },
    });
    expect(result.applied).toBe(false);
    expect(result.reason).toMatch(/^unknown_type:/);
  });
});
