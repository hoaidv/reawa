---
id: STORY-RW-037
title: "Window picker flow"
parent_srs: [SRS-RW-37]
parent_req: [REQ-04]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-37] is added, Then adlc audit reports no orphan for [SRS-RW-37]."
---

# STORY-RW-037 — Window picker flow

Implements [SRS-RW-37](../../../.docs/modules/reawa/features/pen-input-absolute/srs-logic.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-37]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-37].
