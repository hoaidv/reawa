---
id: STORY-EP-086
title: R-tree per container
kind: implement
parent_srs: [SRS-EP-80, SRS-EP-81]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 8
owner: dev
depends_on: [STORY-EP-085]
track: TRACK-009
acceptance_criteria:
  - "Given random forests and random queries, When paint-region, point, rectangle and polygon queries run, Then each result equals a brute-force walk (property test)."
  - "Given random edits, When the trees are checked, Then every cell holds 6 to 16 entries except the root, and an old root held by a test still answers its old query unchanged."
  - "Given prune B, When the property test runs, Then it never hides a cell that has a descendant larger than τ."
  - "Given a root with 100k strokes, When the benchmark runs a point query, Then it visits O(log n + k) cells (SRS-EP-81 probe row)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-086 — R-tree per container

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-80](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries) · [SRS-EP-81](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 4 — R-tree per container (4.1 bulk load and queries, 4.2 path-copy updates, 4.3 both prunes in one descent)](../../document-forest-implementation.md#step-4--r-tree-per-container). The plan step links to the design for each part; this
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
| Depends on | STORY-EP-085 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-086] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
