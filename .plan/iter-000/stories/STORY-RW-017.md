---
id: STORY-RW-017
title: "Menu bar connection status indicators"
parent_srs: [SRS-RW-17]
parent_req: [REQ-02]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-17] is added, Then adlc audit reports no orphan for [SRS-RW-17]."
---

# STORY-RW-017 — Menu bar connection status indicators

Implements [SRS-RW-17](../../../.docs/modules/reawa/features/connection-management/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-17]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-17].
