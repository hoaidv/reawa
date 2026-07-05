---
id: STORY-RW-016
title: "Device configuration controls"
parent_srs: [SRS-RW-16]
parent_req: [REQ-06]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-16] is added, Then adlc audit reports no orphan for [SRS-RW-16]."
---

# STORY-RW-016 — Device configuration controls

Implements [SRS-RW-16](../../../.docs/modules/reawa/features/connection-management/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-16]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-16].
