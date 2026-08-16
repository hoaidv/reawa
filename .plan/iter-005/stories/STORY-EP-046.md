---
id: STORY-EP-046
title: Apply per-end connector styles from toolbar
kind: implement
parent_srs: [SRS-EP-19, SRS-EP-18]
parent_req: [REQ-13]
status: draft
priority: P1
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-045]
acceptance_criteria:
  - "Given a selected connector, When the creator picks an end style, Then that end shows it p95 <=300 ms and the other end is unchanged; one undo reverts."
  - "Given a connector with end styles, When a bound box is dragged, Then styles stay on the correct ends and REQ-09 warp bar holds (0 px jump)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-046 — Apply per-end connector styles from toolbar

TRACK-005. Parent [REQ-13]. [REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends) Path A.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-045 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
