---
id: STORY-RW-051
title: "Backend selection and fallback"
parent_srs: [SRS-RW-51]
parent_req: [REQ-08]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-51] is added, Then adlc audit reports no orphan for [SRS-RW-51]."
---

# STORY-RW-051 — Backend selection and fallback

Implements [SRS-RW-51](../../../.docs/modules/reawa/features/pen-input-native-stylus/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-51]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-51].
