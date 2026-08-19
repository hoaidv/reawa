---
id: STORY-EP-047
title: Recognize and preserve endpoint ink
kind: implement
parent_srs: [SRS-EP-35, SRS-EP-37]
parent_req: [REQ-13]
status: draft
priority: P1
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-045]
acceptance_criteria:
  - "Given ink drawn over an existing connector end, When the stroke ends, Then it is bound as endpoint decoration (0 new free Ink there; 0 second connector) and survives a bound-node drag (0 orphaned samples)."
  - "Given the same stroke over empty canvas or the connector spine (not an end), When the stroke ends, Then it is not stolen as endpoint style."
  - "Given a wrong endpoint-ink recog, When the creator undoes once, Then the stroke is ordinary ink again."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-047 — Recognize and preserve endpoint ink

TRACK-005. Parent [REQ-13]. [REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends) Path B.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-045 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
