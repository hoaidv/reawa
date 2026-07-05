---
id: STORY-RW-048
title: "Multi-display window pick"
parent_srs: [SRS-RW-48]
parent_req: [REQ-04]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-48] is added, Then adlc audit reports no orphan for [SRS-RW-48]."
---

# STORY-RW-048 — Multi-display window pick

Implements [SRS-RW-48](../../../.docs/modules/reawa/features/pen-input-absolute/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-48]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-48].
