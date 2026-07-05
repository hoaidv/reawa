---
id: STORY-RW-060
title: "Fallback on startup failure"
parent_srs: [SRS-RW-60]
parent_req: [REQ-08]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-60] is added, Then adlc audit reports no orphan for [SRS-RW-60]."
---

# STORY-RW-060 — Fallback on startup failure

Implements [SRS-RW-60](../../../.docs/modules/reawa/features/pen-input-native-stylus/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-60]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-60].
