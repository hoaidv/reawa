---
id: STORY-RW-011
title: "Single active connection"
parent_srs: [SRS-RW-11]
parent_req: [REQ-02]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-11] is added, Then adlc audit reports no orphan for [SRS-RW-11]."
---

# STORY-RW-011 — Single active connection

Implements [SRS-RW-11](../../../.docs/modules/reawa/features/connection-management/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-11]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-11].
