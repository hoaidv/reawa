---
id: STORY-RW-030
title: "RelativePenDriver mapping"
parent_srs: [SRS-RW-30]
parent_req: [REQ-03]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-30] is added, Then adlc audit reports no orphan for [SRS-RW-30]."
---

# STORY-RW-030 — RelativePenDriver mapping

Implements [SRS-RW-30](../../../.docs/modules/reawa/features/pen-input-relative/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-30]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-30].
