/**
 * Trace bridge for ADLC (scans .ts; device SoT is C++ under regionsync/).
 * @implements [SRS-EP-02] viewport map, append_ink, coherent refresh
 * @implements [SRS-EP-03] no socket I/O on pen sample hot path
 *
 * Implementation: `epaper/regionsync/region_session.hpp`
 * Host tests: `epaper/tests/run_regionsync_test.sh`
 */
export const EPAPER_REGION_SYNC = {
  headers: [
    "regionsync/viewport_map.hpp",
    "regionsync/doc_store.hpp",
    "regionsync/region_session.hpp",
  ],
  hostTest: "tests/run_regionsync_test.sh",
} as const;
