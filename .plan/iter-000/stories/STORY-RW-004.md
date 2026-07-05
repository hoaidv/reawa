---
id: STORY-RW-004
title: "Notification service (bundled runs only)"
parent_srs: [SRS-RW-04]
parent_req: [REQ-09]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-04] is added, Then adlc audit reports no orphan for [SRS-RW-04]."
---

# STORY-RW-004 — Notification service (bundled runs only)

Implements [SRS-RW-04](../../../.docs/modules/reawa/features/menu-bar-shell/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-04]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-04].
