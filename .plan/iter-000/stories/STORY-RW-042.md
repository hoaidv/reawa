---
id: STORY-RW-042
title: "Stage Manager detection"
parent_srs: [SRS-RW-42]
parent_req: [REQ-04]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-42] is added, Then adlc audit reports no orphan for [SRS-RW-42]."
---

# STORY-RW-042 — Stage Manager detection

Implements [SRS-RW-42](../../../.docs/modules/reawa/features/pen-input-absolute/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-42]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-42].
