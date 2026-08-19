---
id: STORY-EP-049
title: Mid-attachments follow connector warp
kind: implement
parent_srs: [SRS-EP-38, SRS-EP-40]
parent_req: [REQ-14]
status: draft
priority: P1
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-048]
acceptance_criteria:
  - "Given a connector with an attachment, When a bound SmartGroup moves, Then the attachment stays on the spine (t / arc-length preserved) at >=5 Hz partial refresh; on lift pose equals last preview (0 px jump)."
  - "Given that move, When undone, Then connector and attachment return to pre-move pose (+-1 px @ 100%)."
  - "Given a connector with no attachments, When the box moves, Then REQ-09 still holds."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-049 — Mid-attachments follow connector warp

TRACK-005. Parent [REQ-14]. [REQ-14](../../../.docs/modules/epaper/prd.md#connector-attachments)



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-048 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
