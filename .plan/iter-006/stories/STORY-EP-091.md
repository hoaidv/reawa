---
id: STORY-EP-091
title: Undo and redo on the forest
kind: implement
parent_srs: [SRS-EP-07]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 5
owner: dev
depends_on: [STORY-EP-088]
track: TRACK-009
acceptance_criteria:
  - "Given insert, remove, move, resize and reparent, When each is undone and redone, Then the same world geometry and ids are restored."
  - "Given an entry whose target was later changed, When undo runs, Then the whole entry is skipped and consumed (SRS-EP-07 skip rule), with 0 half-applied siblings."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-091 — Undo and redo on the forest

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 9 — Undo on the forest: inverse-op undo (ADR-0032) keyed by ids, not handles; undo of a remove allocates new slots under the same ids](../../document-forest-implementation.md#step-9--undo-on-the-forest). The plan step links to the design for each part; this
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
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-091] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
