---
id: STORY-EP-037
title: Design hand-touch: one-finger pick/move and two-finger pan/zoom
kind: design
parent_srs: [SRS-EP-21, SRS-EP-22, SRS-EP-23, SRS-EP-24]
parent_req: [REQ-10]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-10 states, When the package ships, Then one scene HTML exists per listed journey and ui-spec-gate passes."
  - "Given a finger on a <64 du handle, When shown, Then the spec marks the control pen-only (64 du rule / CHL-0019)."
  - "Given one vs two fingers, When specified, Then one-finger empty is not a pan and two-finger does not start a lasso."
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

# STORY-EP-037 — Design hand-touch: one-finger pick/move and two-finger pan/zoom

TRACK-005. Parent [REQ-10]. [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch)

Package `hand-touch/`. Scenes must cover PRD UI states: finger-hit box; move in progress; anchor no-op; one-finger empty no-op; two-finger pan; pinch; pan vs box-move; chip still hittable.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Parents bound 2026-08-19: [SRS-EP-21] one-finger pick/move · [SRS-EP-22] hand-touch chrome · [SRS-EP-23] finger tool switch · [SRS-EP-24] two-finger viewport. Status **ready** for Wave 1 paint.
