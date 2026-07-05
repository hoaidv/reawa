---
id: STORY-RW-065
title: "Settings log tabs"
parent_srs: [SRS-RW-65]
parent_req: [REQ-07]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-65] is added, Then adlc audit reports no orphan for [SRS-RW-65]."
---

# STORY-RW-065 — Settings log tabs

Implements [SRS-RW-65](../../../.docs/modules/reawa/features/diagnostics/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-65]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-65].
