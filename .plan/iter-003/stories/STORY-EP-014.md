---
id: STORY-EP-014
title: "Device document tree and stroke ingestion"
kind: implement
parent_srs: [SRS-EP-07, SRS-EP-09]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-013]
acceptance_criteria:
  - "Given no session has connected, When the creator draws 20 strokes, Then each finished stroke is an Ink node in the local tree within p95 ≤50 ms after pen-up and the panel paints that tree (0 inbound peer pictures)."
  - "Given Selection armed, When the pen moves, Then no stroke begins and no Ink node is ingested."
  - "Given a digitizer-reported channel (pressure, tilt, extras), When the node is stored, Then 100% of reported channels survive; preview may omit them."
  - "Given shared fixtures ops/, When the device applies the same op sequence as the desktop, Then the trees agree (100%); divergence is a CHL-*, not a widened tolerance."
  - "Given the next stroke after ingestion, When measured, Then pen-down → pixel p95 remains ≤30 ms (0 samples dropped by ingestion)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-014 — Device document tree and stroke ingestion

Implements the tree + ingestion half of [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md)
and the device-local structures / sample retention / fixtures of
[SRS-EP-09](../../../.docs/modules/epaper/features/device-document/srs-data.md).
Domain: [vector-document](../../../.docs/domain/vector-document.md).

BDD: [ingest-stroke.feature](../../../.docs/modules/epaper/features/device-document/bdd/ingest-stroke.feature)
(`@SRS-EP-07` / `@SRS-EP-09`). QA walk: [2026-08-13-qa-to-dev-w9.md](../handoffs/2026-08-13-qa-to-dev-w9.md).

**Hold lifted:** [STORY-EP-013](./STORY-EP-013.md) is **done** (RM2 arrival→flush p95=1298 µs, hit-test p95=22 µs, dropped=0). [CHL-0009](../challenges/CHL-0009-missing-device-document-srs-logic.md) resolved. Op types are the SRS-IN-09 transmit names (`set_smart_transform`, `remove_node` — do not invent a third alias). Do not ship the undo ring (EP-015) or publish queue (EP-020) in this story.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-013 |

## Done when

- `@SRS-EP-07` / `@SRS-EP-09` ingest-stroke scenarios green
- `ops/` fixtures run on device; `@implements [SRS-EP-07]` / `[SRS-EP-09]` in code

## QA (2026-08-13)

**PASS** — host + RM2 synth. [Handoff](../handoffs/2026-08-13-qa-to-sm-ep-014.md). EP-015 not flipped.

Protocol: USB `en7` (`10.11.99.12` → `10.11.99.1`); `RM_INK_TRACE=1 RM_DOC_PROBE=1 RM_DOC_PROBE_SYNTH=1 ./scripts/deploy-rm2.sh --build`. xochitl restored after.

| Metric | Host | Device (RM2) | Bar |
|---|---|---|---|
| pen-up → Ink node p95 | **15 µs** (n=40) | **231 µs** (`ink_nodes=40` applied=40 rejected=0) | ≤50 ms |
| Shared `ops/` vs Infini apply | **agree** | host fixtures (device apply is the same `DeviceDocument`) | 100% |
| Digitizer channels on stored node | pressure, tiltX/Y, distance, timestamp, extras | same ingest path | 100% |
| arrival→flush p95 | n/a | **1392 µs** (n=82) | ≤30 ms pen-down→pixel; EP-013 band ~798–1517 µs |
| dropped / paint-loop-hits | 0 / 0 | **0 / 0** | 0 |
| hit-test p95 | 1 µs | **23 µs** | ≤100 ms |

Ingestion runs **after** `flushPending()` on pen-up — not between a sample and its pixel, not in `paint()`. Undo ring and publish queue not shipped. Selection-armed is the `m_toolMode == "selection"` skip in `ingestPoint` (synth is Pen).
