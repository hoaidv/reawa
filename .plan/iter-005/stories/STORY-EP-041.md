---
id: STORY-EP-041
title: Hardware eraser nib stroke-erase
kind: implement
parent_srs: [SRS-EP-01, SRS-EP-07]
parent_req: [REQ-11]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-040]
acceptance_criteria:
  - "Given a stylus with eraser nib and ink on the panel, When the creator rubs the nib across that ink, Then intersecting samples are gone with p95 <=50 ms after gesture end, 0 new Ink nodes, one undo restores (+-1 px @ 100%)."
  - "Given a stylus without an eraser nib, When the creator uses the pen tip, Then Path A does not fire (0 accidental erases)."
  - "Given no session, When Path A runs, Then the result matches the linked case."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-041 — Hardware eraser nib stroke-erase

TRACK-005. Parent [REQ-11]. [REQ-11](../../../.docs/modules/epaper/prd.md#erase) Path A.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-040 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
