---
id: STORY-EP-038
title: One-finger hit box: select freeform and move
kind: implement
parent_srs: [SRS-EP-21, SRS-EP-23, SRS-EP-25]
parent_req: [REQ-10]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-037, STORY-EP-054]
acceptance_criteria:
  - "Given Pen and a Smart Group at/above LOD, When the creator finger-downs inside the box, Then exclusive tool becomes sel_freeform, the box is selected, chip updates, p95 <=300 ms."
  - "Given that down or a following drag inside the box, When the finger moves, Then the box follows REQ-06 live-direct (0 px jump; >=5 Hz partial) and 0 viewport pan starts."
  - "Given finger-down on a resize knob of a selected box, When the finger moves, Then the box resizes with REQ-06 live-direct (0 px jump; >=5 Hz partial) and 0 viewport pan."
  - "Given one finger on empty canvas with travel at/below 10 mm (89 du), When the touch ends, Then tool is unchanged, 0 nodes selected, 0 pan (palm-rest / tap)."
  - "Given one finger on empty canvas with travel past 10 mm, When the finger moves, Then the local Epaper viewport pans and Infini's camera is unchanged unless Infini follow is on."
  - "Given finger on a 64 du ToolChip tile, When tapped, Then REQ-03 still holds."
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

# STORY-EP-038 — One-finger hit box: select freeform and move

TRACK-005. Parent [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch). Depends on [STORY-EP-037](./STORY-EP-037.md) and [STORY-EP-054](./STORY-EP-054.md) (palm vs empty pan). [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md).



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-037 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
