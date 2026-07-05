---
id: STORY-RW-046
title: "Menu bar Absolute context"
parent_srs: [SRS-RW-46]
parent_req: [REQ-04]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-46] is added, Then adlc audit reports no orphan for [SRS-RW-46]."
---

# STORY-RW-046 — Menu bar Absolute context

Implements [SRS-RW-46](../../../.docs/modules/reawa/features/pen-input-absolute/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-46]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-46].
