---
id: STORY-EP-014
title: "Device document tree and stroke ingestion"
kind: implement
parent_srs: [SRS-EP-07, SRS-EP-09]
parent_req: [REQ-04]
status: blocked
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-013]
blocked_reason: "CHL-0009 — srs-logic.md (SRS-EP-07) not on disk; also waits on STORY-EP-013 latency pass"
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

**Hold:** [CHL-0009](../challenges/CHL-0009-missing-device-document-srs-logic.md) until
`srs-logic.md` exists. **Hold:** [STORY-EP-013](./STORY-EP-013.md) must pass first.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-013 |

## Done when

- Product AC for ingestion + local paint green
- `ops/` fixtures run on device; `@implements [SRS-EP-07]` / `[SRS-EP-09]` in code
