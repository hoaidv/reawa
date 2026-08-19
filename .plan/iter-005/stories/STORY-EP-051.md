---
id: STORY-EP-051
title: Manual insert frame connector primitive
kind: implement
parent_srs: [SRS-EP-45, SRS-EP-46, SRS-EP-48]
parent_req: [REQ-17]
status: draft
priority: P2
iter: iter-005
estimate: 8
owner: dev
depends_on: [STORY-EP-050]
acceptance_criteria:
  - "Given a manual Frame control, When the creator places a frame, Then a Frame node exists at placed bounds (+-1 px @ 100%) with p95 <=300 ms and one undo removes it."
  - "Given two bindable nodes, When the creator manually creates a connector, Then it has the REQ-09 warp contract."
  - "Given a connector, When the creator manually attaches a node, Then REQ-14 holds."
  - "Given a primitive (ellipse/rect/line), When placed, Then paint is primitive geometry (not a polyline stand-in) and survives save/mirror."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-051 — Manual insert frame connector primitive

TRACK-005. Parent [REQ-17]. [REQ-17](../../../.docs/modules/epaper/prd.md#manual-create)



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-050 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
