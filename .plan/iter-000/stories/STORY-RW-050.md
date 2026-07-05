---
id: STORY-RW-050
title: "Close reverts to Relative"
parent_srs: [SRS-RW-50]
parent_req: [REQ-04]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-50] is added, Then adlc audit reports no orphan for [SRS-RW-50]."
---

# STORY-RW-050 — Close reverts to Relative

Implements [SRS-RW-50](../../../.docs/modules/reawa/features/pen-input-absolute/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-50]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-50].
