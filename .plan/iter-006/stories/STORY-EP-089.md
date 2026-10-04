---
id: STORY-EP-089
title: Connector on the forest
kind: implement
parent_srs: [SRS-EP-18]
parent_req: [REQ-09]
status: draft
priority: P1
iter: iter-006
estimate: 5
owner: dev
depends_on: [STORY-EP-088]
track: TRACK-009
acceptance_criteria:
  - "Given every fixture of `epaper_old/tests/connector_warp_test.cpp`, When the new derive runs, Then `route.body` matches the old output."
  - "Given a knob resize, an All-mode ancestor resize or a reparent, When the connector re-derives, Then its departure angle is unchanged."
  - "Given a bound box is deleted, When the forest is painted, Then the connector stays drawn; undo rebinds it through the id; moving a detached connector shifts its route exactly by Δ."
  - "Given a labelled connector, When either end box moves, Then the label stays at its `t` and offset."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-089 — Connector on the forest

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-18](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-18-connector-warp) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 7 — Connector (7.1 payload and derive, 7.2 dependents, missing ends and detached move, 7.3 labels)](../../document-forest-implementation.md#step-7--connector). The plan step links to the design for each part; this
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
| Depends on | STORY-EP-088 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-089] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
