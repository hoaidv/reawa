/**
 * @implements [SRS-IN-17] Device debug-log channel — decode + ring
 * Sidecar TCP :9878 only. Never mix with :9877.
 */

"use strict";

const DEBUG_RING_CAP = 10000;

const DEBUG_LOG_LEVELS = new Set(["info", "warning", "critical", "stdout", "stderr"]);
const DEBUG_LOG_LOGGERS = new Set(["qt", "stdio"]);

/** ADR-0015 / session types — illegal on :9878; drop, never forward to :9877. */
const DOCUMENT_TYPES = new Set([
  "viewport",
  "doc_change",
  "doc_load",
  "doc_snapshot",
  "hello",
  "stroke_begin",
  "stroke_point",
  "stroke_end",
  "drain_ack",
  "queue_empty",
  "load_ack",
  "region_refresh",
  "pickables",
  "tool_intent",
]);

/**
 * @param {string} line
 * @returns {{ kind: "log", record: object } | { kind: "drop", reason: string, type?: string }}
 */
function decodeDebugLine(line) {
  let obj;
  try {
    obj = JSON.parse(line);
  } catch {
    return { kind: "drop", reason: "bad_json" };
  }
  if (!obj || typeof obj !== "object" || Array.isArray(obj)) {
    return { kind: "drop", reason: "not_object" };
  }
  const type = typeof obj.type === "string" ? obj.type : "";
  if (type !== "debug_log") {
    return { kind: "drop", reason: "unknown_type", type };
  }
  const level = DEBUG_LOG_LEVELS.has(obj.level) ? obj.level : "info";
  const logger = DEBUG_LOG_LOGGERS.has(obj.logger) ? obj.logger : "qt";
  let msg = typeof obj.msg === "string" ? obj.msg : String(obj.msg ?? "");
  if (msg.length > 4096) msg = msg.slice(0, 4096);
  const dropped = Number.isFinite(Number(obj.dropped)) ? Math.max(0, Math.floor(Number(obj.dropped))) : 0;
  const ts = Number.isFinite(Number(obj.ts)) ? Number(obj.ts) : Date.now();
  return {
    kind: "log",
    record: { type: "debug_log", ts, level, logger, msg, dropped },
  };
}

function isDocumentTypeOnDebugPort(type) {
  return DOCUMENT_TYPES.has(type);
}

/**
 * @template T
 * @param {T[]} ring
 * @param {T} rec
 * @param {number} [cap]
 * @returns {number} count of oldest records dropped
 */
function pushDebugRing(ring, rec, cap = DEBUG_RING_CAP) {
  ring.push(rec);
  const overflow = ring.length - cap;
  if (overflow > 0) {
    ring.splice(0, overflow);
    return overflow;
  }
  return 0;
}

function controlLine(type) {
  return `${JSON.stringify({ type })}\n`;
}

module.exports = {
  DEBUG_RING_CAP,
  DOCUMENT_TYPES,
  decodeDebugLine,
  isDocumentTypeOnDebugPort,
  pushDebugRing,
  controlLine,
};
