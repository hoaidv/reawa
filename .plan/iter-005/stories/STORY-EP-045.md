---
id: STORY-EP-045
title: Design connector endpoint style toolbar
kind: design
parent_srs: [SRS-EP-36, SRS-EP-34]
parent_req: [REQ-13]
status: draft
priority: P1
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-13 UI states, When the package ships, Then one scene per state and ui-spec-gate passes."
  - "Given a connector, When an end style is chosen, Then the spec shows that end only changing."
design_package: ".plan/iter-005/design/connector-ends/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-045 — Design connector endpoint style toolbar

TRACK-005. Parent [REQ-13]. [REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends)

Package `connector-ends/`. Closed style set: star, empty arrow, fill arrow, one, many (plus Off). Per-end. States: post-create toolbar; selected per-end styles; endpoint-ink accepted/refused; warp with decorated ends.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
