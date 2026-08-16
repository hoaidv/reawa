---
id: STORY-EP-052
title: Barrel click vs hold-move dispatch from catalogue
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-18]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-IN-034]
acceptance_criteria:
  - "Given a 1-button pen and defaults, When the creator clicks (no move), Then exclusive tool toggles Pen <-> sel_freeform p95 <=300 ms and 0 hold-move runs."
  - "Given the same pen, When hold-move, Then temporary freeform runs until release and 0 click fires on release."
  - "Given a 2-button pen and defaults, When B2 hold-moves, Then temporary erase runs until release."
  - "Given Hold-move rebound to drag-under-tip, When starting on a hittable node, Then that node moves (REQ-06); when starting on empty, Then 0 move and 0 lasso."
  - "Given a 0-button pen, When drawing, Then 0 button gestures fire and REQ-03 still works."
  - "Given a 20-gesture fixture mixing clicks and holds, When executed, Then 0 events fire both click and hold-move."
  - "Given a rebind mid-session, When a gesture is in flight, Then the in-flight binding does not change."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-052 — Barrel click vs hold-move dispatch from catalogue

TRACK-005. Parent [REQ-18]. [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons)



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-034 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
