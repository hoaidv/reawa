---
id: STORY-RW-041
title: "Window follow timer"
parent_srs: [SRS-RW-41]
parent_req: [REQ-04]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-41] is added, Then adlc audit reports no orphan for [SRS-RW-41]."
---

# STORY-RW-041 — Window follow timer

Implements [SRS-RW-41](../../../.docs/modules/reawa/features/pen-input-absolute/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-41]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-41].
