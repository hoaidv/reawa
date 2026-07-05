---
id: STORY-RW-002
title: "Menu bar icon and menu rebuild"
parent_srs: [SRS-RW-02]
parent_req: [REQ-01]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-02] is added, Then adlc audit reports no orphan for [SRS-RW-02]."
---

# STORY-RW-002 — Menu bar icon and menu rebuild

Implements [SRS-RW-02](../../../.docs/modules/reawa/features/menu-bar-shell/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-02]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-02].
