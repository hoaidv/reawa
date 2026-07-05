---
id: STORY-RW-007
title: "Startup — non-bundled launch"
parent_srs: [SRS-RW-07]
parent_req: [REQ-01]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-07] is added, Then adlc audit reports no orphan for [SRS-RW-07]."
---

# STORY-RW-007 — Startup — non-bundled launch

Implements [SRS-RW-07](../../../.docs/modules/reawa/features/menu-bar-shell/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-07]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-07].
