---
id: STORY-RW-033
title: "Relative mode selection"
parent_srs: [SRS-RW-33]
parent_req: [REQ-03]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-33] is added, Then adlc audit reports no orphan for [SRS-RW-33]."
---

# STORY-RW-033 — Relative mode selection

Implements [SRS-RW-33](../../../.docs/modules/reawa/features/pen-input-relative/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-33]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-33].
