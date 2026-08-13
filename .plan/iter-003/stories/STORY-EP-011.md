---
id: STORY-EP-011
title: Live direct resize — drop ghost
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — device-side behaviour re-specified under epaper REQ-04…REQ-07; code discarded by the restore. Not scheduled; SM re-slices."
priority: P1
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-IN-025]
acceptance_criteria:
  - "Given fixedInk resize drag on tablet, when dragging, then baked member ink updates in place (no dashed ink ghost); content sample size stays fixed; boundary AABB-maps"
  - "Given resize drag, when throttled, then tool_intent resize live:true is emitted with world AABB"
  - "Given pen-up, when gesture ends, then commit tool_intent without live is emitted"
---

# STORY-EP-011 — Live direct resize

[SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md) · CHL-0006
