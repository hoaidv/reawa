---
id: STORY-EP-038
title: One-finger hit box: select freeform and move
kind: implement
parent_srs: [SRS-EP-11, SRS-EP-04]
parent_req: [REQ-10]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-037]
acceptance_criteria:
  - "Given Pen and a Smart Group at/above LOD, When the creator finger-downs inside the box, Then exclusive tool becomes sel_freeform, the box is selected, chip updates, p95 <=300 ms."
  - "Given that down or a following drag inside the box, When the finger moves, Then the box follows REQ-06 live-direct (0 px jump; >=5 Hz partial) and 0 viewport pan starts."
  - "Given finger-down on a resize anchor (<64 du), When the touch ends, Then 0 resize starts; pen on the same anchor still resizes."
  - "Given one finger on empty canvas, When the touch ends, Then tool is unchanged and 0 nodes are selected (0 lasso, 0 pan)."
  - "Given finger on a 64 du ToolChip tile, When tapped, Then REQ-03 still holds."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-038 — One-finger hit box: select freeform and move

TRACK-005. Parent [REQ-10]. [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) · depends_on EP-037



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-037 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
