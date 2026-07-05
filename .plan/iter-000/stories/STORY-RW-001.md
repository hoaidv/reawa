---
id: STORY-RW-001
title: "Application activation policy"
parent_srs: [SRS-RW-01]
parent_req: [REQ-01]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-01] is added, Then adlc audit reports no orphan for [SRS-RW-01]."
---

# STORY-RW-001 — Application activation policy

Implements [SRS-RW-01](../../../.docs/modules/reawa/features/menu-bar-shell/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-01]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-01].
