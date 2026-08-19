---
id: STORY-EP-050
title: Design manual create: frame connector primitive
kind: design
parent_srs: [SRS-EP-47, SRS-EP-44]
parent_req: [REQ-17]
status: draft
priority: P2
iter: iter-005
estimate: 5
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-17 UI states, When the package ships, Then one scene per state and ui-spec-gate passes."
  - "Given the ToolChip, When manual create is specified, Then it is not a fourth exclusive ink tool that replaces Pen (closed insert set)."
design_package: ".plan/iter-005/design/manual-create/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-050 — Design manual create: frame connector primitive

TRACK-005. Parent [REQ-17]. [REQ-17](../../../.docs/modules/epaper/prd.md#manual-create). Not a general palette.

Package `manual-create/`. Closed insert set: frame, connector, attachment, primitive (ellipse/rect/line). Ink-box stays REQ-05. States: entry; frame place; connector place; primitive place; cancel; vs Pen ink.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
