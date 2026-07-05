---
id: STORY-RW-025
title: "Discovered devices list"
parent_srs: [SRS-RW-25]
parent_req: [REQ-05]
status: done
priority: P1
iter: iter-000
estimate: 1
owner: dev
acceptance_criteria:
  - "Given existing Swift code, When @implements [SRS-RW-25] is added, Then adlc audit reports no orphan for [SRS-RW-25]."
---

# STORY-RW-025 — Discovered devices list

Implements [SRS-RW-25](../../../.docs/modules/reawa/features/device-discovery/srs-ui.md).

Retroactive traceability story — implementation pre-existed this iter.

## Done When
- `@implements [SRS-RW-25]` present in `Sources/`.
- Sync-Auditor reports no orphan for [SRS-RW-25].
