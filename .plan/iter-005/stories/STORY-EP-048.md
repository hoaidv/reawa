---
id: STORY-EP-048
title: Design connector mid-attachments
kind: design
parent_srs: [SRS-EP-19]
parent_req: [REQ-14]
status: draft
priority: P1
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-14 UI states, When the package ships, Then one scene per state and ui-spec-gate passes."
  - "Given an attachment on a connector, When a bound box moves, Then the spec shows the attachment riding the spine (not world-offset drift)."
design_package: ".plan/iter-005/design/connector-attach/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-048 — Design connector mid-attachments

TRACK-005. Parent [REQ-14]. [REQ-14](../../../.docs/modules/epaper/prd.md#connector-attachments)

Package `connector-attach/`. States: place/bind attachment; selected attachment on connector; drag bound box with attachments; empty connector.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
