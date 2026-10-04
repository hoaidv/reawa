---
id: STORY-EP-090
title: Progressive paint: tiles, jobs, coarse pass
kind: implement
parent_srs: [SRS-EP-02, SRS-EP-13]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 8
owner: dev
depends_on: [STORY-EP-087]
track: TRACK-009
acceptance_criteria:
  - "Given a pan, When the buffer shifts, Then only newly exposed tiles are rendered (counted in a test)."
  - "Given edits stop, When the queue drains, Then every tile equals a fresh single-threaded render (pixel diff)."
  - "Given a dense document on the device, When slice time, first present and refresh count are measured, Then the results are recorded and τ, slice length and pad are confirmed or tuned in rendering.md."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-090 — Progressive paint: tiles, jobs, coarse pass

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md) · [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-13-device-document-quality) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 8 — Progressive paint (8.1 tiles and padded buffer, 8.2 job queue, epochs and slices, 8.3 coarse pass and e-ink present)](../../document-forest-implementation.md#step-8--progressive-paint). The plan step links to the design for each part; this
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
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-090] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
