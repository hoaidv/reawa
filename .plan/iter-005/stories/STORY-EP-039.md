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
  - "Given two fingers on empty canvas and no box-move or resize in flight, When pan or pinch runs >=5 s, Then the next pen sample uses the new local region with p95 map apply <=100 ms."
  - "Given Infini follow is off, When the tablet two-finger pans, Then Infini's camera is unchanged (independent default)."
  - "Given Infini follow is on, When the tablet two-finger pans, Then Infini view matches after settle — peer IN-033."
  - "Given a one-finger box-move or resize in flight, When a second finger lands, Then 0 pan starts until that gesture ends."
  - "Given the link down, When the creator two-finger pans, Then local viewport still changes."
  - "Given Epaper is following Infini, When the creator two-finger pans on Epaper, Then Epaper follow turns off (follower local-nav)."
design_package: ".plan/iter-005/design/hand-touch/"
ui_spec: ".plan/iter-005/design/hand-touch/ui-spec.md"
scenes:
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-moving.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-resizing.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-one-finger-empty.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-two-finger-pan.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pinch.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pan-vs-move.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-link-down-local-view.html"
hifi: ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
wireframe: ""
---

# STORY-EP-039 — Two-finger pan and pinch; publish viewport

TRACK-005. Parent [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) two-finger **local** Must. Publish **only if** Infini follow is on ([ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md)). BRD-07 ship gate **lifted** 2026-08-20.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-037 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
