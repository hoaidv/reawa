---
id: STORY-RW-005
title: "Menu structure"
parent_srs: [SRS-RW-05]
parent_req: [REQ-01]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-05] is added, Then adlc audit reports no orphan for [SRS-RW-05]."
---

# STORY-RW-005 — Menu structure

Implements [SRS-RW-05](../../../.docs/modules/reawa/features/menu-bar-shell/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-05]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-05].
