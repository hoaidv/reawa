---
id: STORY-RW-049
title: "Stage Manager R1/R2 scenarios"
parent_srs: [SRS-RW-49]
parent_req: [REQ-04]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-49] is added, Then adlc audit reports no orphan for [SRS-RW-49]."
---

# STORY-RW-049 — Stage Manager R1/R2 scenarios

Implements [SRS-RW-49](../../../.docs/modules/reawa/features/pen-input-absolute/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-49]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-49].
