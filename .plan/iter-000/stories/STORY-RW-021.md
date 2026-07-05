---
id: STORY-RW-021
title: "SSH host probing"
parent_srs: [SRS-RW-21]
parent_req: [REQ-05]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-21] is added, Then adlc audit reports no orphan for [SRS-RW-21]."
---

# STORY-RW-021 — SSH host probing

Implements [SRS-RW-21](../../../.docs/modules/reawa/features/device-discovery/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-21]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-21].
