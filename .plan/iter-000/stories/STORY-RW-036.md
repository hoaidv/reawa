---
id: STORY-RW-036
title: "AbsolutePenDriver mapping"
parent_srs: [SRS-RW-36]
parent_req: [REQ-04]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-36] is added, Then adlc audit reports no orphan for [SRS-RW-36]."
---

# STORY-RW-036 — AbsolutePenDriver mapping

Implements [SRS-RW-36](../../../.docs/modules/reawa/features/pen-input-absolute/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-36]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-36].
