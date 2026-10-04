---
id: STORY-EP-087
title: Concurrency: granules, commit, reclamation
kind: implement
parent_srs: [SRS-EP-07, SRS-EP-80, SRS-EP-81]
parent_req: [REQ-04]
status: draft
priority: P1
iter: iter-006
estimate: 8
owner: dev
depends_on: [STORY-EP-086]
track: TRACK-009
acceptance_criteria:
  - "Given a reader loop copying headers and walking children under a writer loop, When ThreadSanitizer runs, Then no torn read is reported."
  - "Given a damage message, When a render job starts after it, Then the job sees that commit."
  - "Given one writer churning slots and two readers looping, When the stress test runs under AddressSanitizer and ThreadSanitizer, Then nothing is reported."
  - "Given a writer committing every 5 ms, When a reader runs a membership query, Then it terminates (at most 3 retries, then handed to the writer)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---
# STORY-EP-087 — Concurrency: granules, commit, reclamation

<!-- This ID identifies the work, independently of document headings or file position. -->
Implements [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) · [SRS-EP-80](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries) · [SRS-EP-81](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality) on the document forest
([ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md)). Track:
[TRACK-009](../../tracks/TRACK-009-document-forest.md) (Document forest).

**Plan:** [Step 5 — Concurrency (5.1 granule rules, 5.2 commit protocol and damage, 5.3 epoch-based reclamation, 5.4 exact queries from a reader). Fallback: reader–writer lock behind the same API, architect's call](../../document-forest-implementation.md#step-5--concurrency). The plan step links to the design for each part; this
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
| Depends on | STORY-EP-086 |

## Done when (implement)

- Every acceptance criterion has a named host test (or a recorded device measurement) that passes.
- File headers carry `@implements [SRS-EP-NN] <short label>` for the parent SRS, or `[STORY-EP-087] <short label>` where no SRS fits ([traceability](../../../.agent/rules/traceability.md)). Tests name this story in their file header.
- No Qt include under `epaper/src/doc/`.
