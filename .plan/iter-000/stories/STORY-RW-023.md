---
id: STORY-RW-023
title: "Auto-connect and notification matrix"
parent_srs: [SRS-RW-23]
parent_req: [REQ-05]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-23] is added, Then adlc audit reports no orphan for [SRS-RW-23]."
---

# STORY-RW-023 — Auto-connect and notification matrix

Implements [SRS-RW-23](../../../.docs/modules/reawa/features/device-discovery/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-23]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-23].
