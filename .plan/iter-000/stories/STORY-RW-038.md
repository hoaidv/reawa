---
id: STORY-RW-038
title: "WindowSnapController"
parent_srs: [SRS-RW-38]
parent_req: [REQ-04]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-38] is added, Then adlc audit reports no orphan for [SRS-RW-38]."
---

# STORY-RW-038 — WindowSnapController

Implements [SRS-RW-38](../../../.docs/modules/reawa/features/pen-input-absolute/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-38]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-38].
