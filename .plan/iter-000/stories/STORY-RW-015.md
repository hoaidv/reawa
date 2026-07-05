---
id: STORY-RW-015
title: "Settings connection editor"
parent_srs: [SRS-RW-15]
parent_req: [REQ-06]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-15] is added, Then adlc audit reports no orphan for [SRS-RW-15]."
---

# STORY-RW-015 — Settings connection editor

Implements [SRS-RW-15](../../../.docs/modules/reawa/features/connection-management/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-15]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-15].
