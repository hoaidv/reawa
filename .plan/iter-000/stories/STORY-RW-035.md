---
id: STORY-RW-035
title: "External cursor interference recovery"
parent_srs: [SRS-RW-35]
parent_req: [REQ-03]
status: ready
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-35] is added, Then adlc audit reports no orphan for [SRS-RW-35]."
---

# STORY-RW-035 — External cursor interference recovery

Implements [SRS-RW-35](../../../.docs/modules/reawa/features/pen-input-relative/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-35]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-35].
