---
id: STORY-EP-084
title: Nodes, children, bounds and invariants
kind: implement
parent_srs: [SRS-EP-07]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 5
owner: dev
depends_on: [STORY-EP-083]
track: TRACK-009
acceptance_criteria:
  - "Given each kind, When it is constructed, read and destroyed in a test, Then it works, and a payload of one kind cannot be read as another."
  - "Given insert, remove, reorder and reparent, When each runs, Then tests pass, a reparented node keeps its handle, and a removed container retires its whole subtree."
  - "Given an ink box is moved, When the commit closes, Then exactly one entry above it changes; boundary-ink overhang widens only the paint extent."
  - "Given a randomized edit sequence (insert, remove, reparent, move), When 10,000 commits run in a debug build, Then the invariants checker reports no violation."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-084 — Nodes, children, bounds and invariants

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 3.1–3.4 — node header and payloads, `Children` with link / unlink / retire, child space and bounds, invariants checker](../../document-forest-implementation.md#step-3--nodes-and-children). The plan step links to the design for each part; this
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
| Depends on | STORY-EP-083 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-084] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
