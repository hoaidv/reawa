---
id: STORY-EP-044
title: In-document copy cut paste ops
kind: implement
parent_srs: [SRS-EP-31, SRS-EP-33]
parent_req: [REQ-12]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-043]
acceptance_criteria:
  - "Given a non-empty selection, When copy then paste, Then a new subtree exists with new ids, geometry = source + documented offset (+-1 px @ 100%), source unchanged."
  - "Given a non-empty selection, When cut then paste, Then originals are gone after cut and paste matches; one undo of paste removes copies; second undo restores originals."
  - "Given empty clipboard, When paste is invoked, Then 0 nodes change."
  - "Given no session, When copy/cut/paste runs, Then behaviour matches the linked case."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-044 — In-document copy cut paste ops

TRACK-005. Parent [REQ-12]. [REQ-12](../../../.docs/modules/epaper/prd.md#clipboard)



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-043 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
