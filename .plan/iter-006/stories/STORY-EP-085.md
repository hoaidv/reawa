---
id: STORY-EP-085
title: Milestone A: paint pen ink from the forest
kind: implement
parent_srs: [SRS-EP-07, SRS-EP-01]
parent_req: [REQ-04, REQ-01]
status: draft
priority: P1
iter: iter-006
estimate: 5
owner: dev
depends_on: [STORY-EP-084]
track: TRACK-009
acceptance_criteria:
  - "Given the device, When the creator draws and lifts the pen, Then the stroke persists after pen-up and redraws from the forest after a full refresh."
  - "Given the commit before this story as the baseline, When pen latency is measured on the device, Then p95 pen-down to pixel shows no regression (SRS-EP-01 bar ≤30 ms)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-085 — Milestone A: paint pen ink from the forest

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) · [SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 3.5 — Milestone A: the existing pen path commits each stroke as an `Ink` under `Document`; the canvas paints by a synchronous, single-threaded visit of the forest](../../document-forest-implementation.md#35-milestone-a-paint-pen-ink-from-the-forest). The plan step links to the design for each part; this
story does not restate it.

## Architectural change trace

- Source: [ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md), records settled in
  [CHL-0033](../challenges/CHL-0033-forest-product-records.md).
- Implementation: pending — the human writes the code; agents guide and review.
- Verification: pending — the acceptance criteria above, as named `ctest` cases (or device
  measurements where stated).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner persona | `dev` (the human, with agent guidance) |
| Depends on | STORY-EP-084 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-085] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
