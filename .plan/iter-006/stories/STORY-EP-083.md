---
id: STORY-EP-083
title: Slot table, handles and NodeStore
kind: implement
parent_srs: [SRS-EP-07]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 3
owner: dev
depends_on: [STORY-EP-082]
track: TRACK-009
acceptance_criteria:
  - "Given a slot table growing across a chunk boundary, When addresses are compared before and after, Then every existing slot address is unchanged."
  - "Given the slot-7 worked example, When it runs as a test, Then a stale handle resolves to `Gone` after reuse, and a slot at `gen = 2^32 - 1` is never reused."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-083 — Slot table, handles and NodeStore

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 2 — Slots and handles (2.1 slot table, 2.2 `NodeStore`, allocate and free)](../../document-forest-implementation.md#step-2--slots-and-handles). The plan step links to the design for each part; this
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
| Depends on | STORY-EP-082 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-083] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
