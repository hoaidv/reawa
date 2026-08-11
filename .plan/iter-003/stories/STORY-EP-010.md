---
id: STORY-EP-010
title: fixedInk mode-correct resize ghost
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: in-review
priority: P1
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-IN-025]
acceptance_criteria:
  - "Given fixedInk pickable resize drag, when ghost paints, then content member paths do not scale; boundary members scale with AABB"
  - "Given withBounds resize, when ghost paints, then all members scale+translate"
  - "Given move gesture, when ghost paints, then all members translate only"
---

# STORY-EP-010 — fixedInk mode-correct resize ghost

[SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md) · CHL-0005

## Done when

- `paintInkGhost` uses pickable `inkScaleMode` + `members`
- Human verify: tablet resize stub content size stays fixed under Fixed ink
