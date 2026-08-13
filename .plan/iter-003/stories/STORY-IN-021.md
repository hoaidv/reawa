---
id: STORY-IN-021
title: "Fixed-corner Smart Group resize"
kind: implement
parent_srs: [SRS-IN-11]
parent_req: [REQ-04]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — assumes desktop tree authority; behaviour re-specified under epaper REQ-06. Not scheduled; SM re-slices."
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given resize from sw corner, When dragging, Then ne corner stays fixed in world space."
  - "Given fixedInk or withBounds mode, When resizing, Then opposite corner/edge pinned (ml-mindmap pattern)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-021 — Fixed-corner Smart Group resize

`resizeWorldAabbFromHandle` + `smartTransformFromWorldAabb` in `selection.ts`.
