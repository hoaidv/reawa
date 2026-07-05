---
id: STORY-RW-006
title: "Dock visibility states"
parent_srs: [SRS-RW-06]
parent_req: [REQ-01]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-06] is added, Then adlc audit reports no orphan for [SRS-RW-06]."
---

# STORY-RW-006 — Dock visibility states

Implements [SRS-RW-06](../../../.docs/modules/reawa/features/menu-bar-shell/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-06]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-06].
