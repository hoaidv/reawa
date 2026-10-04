---
id: STORY-EP-088
title: Manipulation core: move, resize, reparent
kind: implement
parent_srs: [SRS-EP-07]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 8
owner: dev
depends_on: [STORY-EP-087]
track: TRACK-009
acceptance_criteria:
  - "Given each kind, When move or resize is called through the single entry point, Then the kind's response runs, and a knob below the minimum size clamps."
  - "Given nested ink boxes in All mode, When the outer one is resized, Then each box applies its own mode."
  - "Given a group, When it is resized, Then circles stay round; When it is moved, Then only `origin` is written."
  - "Given Boundary mode, When the box is resized, Then content follows the top-left corner."
  - "Given a box moved into a nested box, When the reparent commits, Then every world sample position is unchanged and the node keeps its handle."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-088 — Manipulation core: move, resize, reparent

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 6 — Manipulation (6.1 contract and gating, 6.2 per-kind responses, 6.3 multi-selection and reparent). Engine only: no product verb or chrome; REQ-06 is deprecated and REQ-08 parked (CHL-0033)](../../document-forest-implementation.md#step-6--manipulation). The plan step links to the design for each part; this
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
| Depends on | STORY-EP-087 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-088] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
