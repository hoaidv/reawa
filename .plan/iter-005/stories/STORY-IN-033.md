---
id: STORY-IN-033
title: Infini follows tablet-published viewport
kind: implement
parent_srs: [SRS-IN-20, SRS-IN-21, SRS-IN-22]
parent_req: [REQ-03]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-039]
acceptance_criteria:
  - "Given a tablet two-finger pan/pinch, When the viewport message arrives, Then Infini canvas matches after settle (0 divergent viewports)."
  - "Given Infini idle, When the tablet is panning, Then Infini sends 0 competing viewport bursts that fight the gesture (last-writer ADR)."
  - "Given Infini user pans, When that is the last writer, Then tablet still applies Infini viewport (REQ-02) after the tablet gesture ends."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-033 — Infini follows tablet-published viewport

TRACK-005. Parent [REQ-03]. Peer of EP-039. Infini [REQ-03](../../../.docs/modules/infini/prd.md#tablet-sync) / [REQ-01](../../../.docs/modules/infini/prd.md#infinity-canvas).



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-039 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
