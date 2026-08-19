---
id: STORY-EP-042
title: Selection-erase deletes selected nodes
kind: implement
parent_srs: [SRS-EP-28, SRS-EP-30]
parent_req: [REQ-11]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-040]
acceptance_criteria:
  - "Given a non-empty selection, When Erase is invoked, Then every selected node is removed (0 leftovers on next settled frame) and one undo restores them."
  - "Given empty selection, When Erase is invoked, Then 0 nodes change."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-042 — Selection-erase deletes selected nodes

TRACK-005. Parent [REQ-11]. [REQ-11](../../../.docs/modules/epaper/prd.md#erase) Path B.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-040 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
