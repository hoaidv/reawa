---
id: STORY-EP-082
title: Forest foundations: host tests and geometry
kind: implement
parent_srs: [SRS-EP-07]
parent_req: [REQ-04]
status: ready
priority: P1
iter: iter-006
estimate: 3
owner: dev
depends_on: []
track: TRACK-009
acceptance_criteria:
  - "Given `epaper_doc` (no Qt) and a host test executable, When `ctest` runs in the plain, thread-sanitizer and address-sanitizer presets, Then an empty test passes in all three and the reMarkable 2 build still links."
  - "Given the geometry types, When unit tests run, Then box union and intersection, the empty box, `ResizeMap` from each of the 8 knobs (opposite side fixed) and the panel map round trip against the cases of `epaper_old/tests/canvas_frame_test.cpp` pass."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-082 — Forest foundations: host tests and geometry

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 1 — Foundations (1.1 host test target and `epaper_doc`, 1.2 geometry types)](../../document-forest-implementation.md#step-1--foundations). The plan step links to the design for each part; this
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
| Depends on | none |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-082] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
