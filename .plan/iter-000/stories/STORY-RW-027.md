---
id: STORY-RW-027
title: "USB subnet discovery"
parent_srs: [SRS-RW-27]
parent_req: [REQ-05]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-27] is added, Then adlc audit reports no orphan for [SRS-RW-27]."
---

# STORY-RW-027 — USB subnet discovery

Implements [SRS-RW-27](../../../.docs/modules/reawa/features/device-discovery/srs-quality.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-27]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-27].
