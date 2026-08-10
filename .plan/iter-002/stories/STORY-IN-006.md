---
id: STORY-IN-006
title: "Design Infini document open save chrome"
kind: design
parent_srs: [SRS-IN-05]
parent_req: [REQ-02]
status: draft
priority: P2
iter: iter-002
estimate: 2
owner: designer
depends_on: []
acceptance_criteria:
  - "Given states doc.none, doc.open, doc.dirty, doc.error, When the package ships, Then each has a scene or annotated state in ui-spec."
  - "Given vertical WIP=1 on infinity-canvas, When this story is worked, Then it stays draft/queued until F1 design+implement wave clears (do not set in-progress in parallel with F1)."
design_package: ".plan/iter-002/design/vector-document/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-006 — Design Infini document open save chrome

Queued design for [SRS-IN-05](../../../.docs/modules/infini/features/vector-document/srs-ui.md)
([REQ-02](../../../.docs/modules/infini/prd.md#vector-document)).

**Not NOW** — vertical `wip: 1` keeps F1 (`infinity-canvas`) as the only feature in flight.
Leave `draft` until F1 campaign slice validates or SM advances the board wave.
