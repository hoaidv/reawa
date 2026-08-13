/**
 * @implements [SRS-IN-18] Device Log filter — view-only substring
 */

export type DebugLogRecord = {
  type: "debug_log";
  ts: number;
  level: string;
  logger: string;
  msg: string;
  dropped: number;
};

/** Case-insensitive substring on msg and level. Empty needle → all records. */
export function filterDebugLog(records: readonly DebugLogRecord[], needle: string): DebugLogRecord[] {
  const q = needle.trim().toLowerCase();
  if (!q) return [...records];
  return records.filter((r) => r.msg.toLowerCase().includes(q) || r.level.toLowerCase().includes(q));
}
