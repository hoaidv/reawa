---
id: STORY-EP-013
title: "Measure ink latency with a resident document"
kind: implement
parent_srs: [SRS-EP-13]
parent_req: [REQ-04]
status: ready
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given the current local-ink paint path (SRS-EP-01 baseline), When a resident document of 500 ink nodes / 50k samples plus a hit-test probe is on the paint path, Then pen-down → pixel p95 is ≤30 ms and equal to the pre-document baseline within measurement error."
  - "Given the same fixture, When hit-test runs, Then hit-test p95 is ≤100 ms and 0 ink samples are dropped or delayed by the probe."
  - "Given the budget is missed, When the story closes, Then a CHL-* is filed against ADR-0014 — not a workaround and not a relaxed ink budget."
  - "Given instrumentation, When it is added, Then it lives outside the paint loop (SRS-EP-13 Notes) so measurement itself cannot break the budget."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-013 — Measure ink latency with a resident document

Spike for [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md)
(the floor that outranks everything) and ADR-0014 Risks row 1. **No REQ-04 implement story
starts until this passes.** Does not ship the tree — stubs a resident document + hit-test on
the existing paint path.

[SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) ≤30 ms is the bar.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- Baseline vs probe numbers recorded in the story or iter memory
- Pass → [STORY-EP-014](./STORY-EP-014.md) may leave `draft`
- Fail → `CHL-*`, stop the rework, do not proceed
