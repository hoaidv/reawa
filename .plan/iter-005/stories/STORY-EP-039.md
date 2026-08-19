---
id: STORY-EP-039
title: Two-finger pan and pinch; publish viewport
kind: implement
parent_srs: [SRS-EP-24, SRS-EP-26]
parent_req: [REQ-10]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-037]
acceptance_criteria:
  - "Given two fingers on empty canvas and no box-move in flight, When pan or pinch runs >=5 s, Then the next pen sample uses the new region with p95 map apply <=100 ms."
  - "Given a live session, When the tablet viewport changes from this gesture, Then Infini view matches after settle (0 divergent viewports) — peer IN-033."
  - "Given a one-finger box-move in flight, When a second finger lands, Then 0 pan starts until the move ends."
  - "Given the link down, When the creator two-finger pans, Then local viewport still changes."
design_package: ".plan/iter-005/design/hand-touch/"
ui_spec: ".plan/iter-005/design/hand-touch/ui-spec.md"
scenes:
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-moving.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-anchor-noop.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-one-finger-empty.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-two-finger-pan.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pinch.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pan-vs-move.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-link-down-local-view.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pen-resize-after-finger-select.html"
hifi: ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
wireframe: ""
---

# STORY-EP-039 — Two-finger pan and pinch; publish viewport

TRACK-005. Parent [REQ-10]. [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) two-finger slice. BRD-07 amendment still open — do not ship this story as Must until analyst updates BRD.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-037 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
