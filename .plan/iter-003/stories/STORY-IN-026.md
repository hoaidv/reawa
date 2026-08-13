---
id: STORY-IN-026
title: Live resize tool_intent + undo origin
kind: implement
parent_srs: [SRS-IN-13]
parent_req: [REQ-03]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — tool_intent transport retires with SRS-IN-13. Not scheduled; SM re-slices."
priority: P1
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given tool_intent resize with live:true, when applied, then Smart Group transform updates without a new undo entry per sample"
  - "Given first live of a gesture then commit, when undo runs, then tree restores pre-gesture geometry"
  - "Given live resize, when applied, then desktop rebuilds so ink tracks the tablet"
---

# STORY-IN-026 — Live resize tool_intent

[SRS-IN-13](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) · CHL-0006
