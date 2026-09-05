---
id: STORY-EP-074
title: Nested ink-box RenderingContext and tap-select
kind: implement
parent_srs: [SRS-EP-76, SRS-EP-77, SRS-EP-11]
parent_req: [REQ-06]
status: in-review
priority: P0
iter: iter-005
estimate: 8
owner: dev
depends_on: []
acceptance_criteria:
  - "Given an ink-box nested in another (paste or enclose), When the creator taps the child, Then the child is selected and the context toolbar is the child’s."
  - "Given that nested child, When the creator moves or resizes it, Then only the child’s own-transform changes (parent pose unchanged except Rule 5)."
  - "Given a nested child, When the camera pans or zooms, Then the child stays painted and remains hittable."
  - "Given sel_rect or sel_freeform over a nested cluster, When the gesture commits, Then nested children are not independently selected."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-074 — Nested ink-box RenderingContext and tap-select

TRACK-005. [CHL-0032](../challenges/CHL-0032-nested-ink-box.md). Logic
[SRS-EP-76](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-76-nested-render) /
[SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent).
Decision [ADR-0039](../../../.docs/adr/ADR-0039-nested-ink-box-rendering.md). **No** design `depends_on`.

Fixes paste-into-box “visible but dead” children.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |
