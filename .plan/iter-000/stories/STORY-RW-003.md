---
id: STORY-RW-003
title: "Accessibility permission gate"
parent_srs: [SRS-RW-03]
parent_req: [REQ-09]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-03] is added, Then adlc audit reports no orphan for [SRS-RW-03]."
---

# STORY-RW-003 — Accessibility permission gate

Implements [SRS-RW-03](../../../.docs/modules/reawa/features/menu-bar-shell/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-03]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-03].
