---
id: STORY-EP-038
title: One-finger hit box: select freeform and move
kind: implement
parent_srs: [SRS-EP-21, SRS-EP-23, SRS-EP-25]
parent_req: [REQ-10]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-037, STORY-EP-054]
acceptance_criteria:
  - "Given Pen and a Smart Group at/above LOD, When the creator finger-downs inside the box, Then exclusive tool becomes sel_freeform, the box is selected, chip updates, p95 <=300 ms."
  - "Given that down or a following drag inside the box, When the finger moves, Then the box follows REQ-06 live-direct (0 px jump; >=5 Hz partial) and 0 viewport pan starts."
  - "Given finger-down on a resize knob of a selected box, When the finger moves, Then the box resizes with REQ-06 live-direct (0 px jump; >=5 Hz partial) and 0 viewport pan."
  - "Given one finger on empty canvas with travel at/below 20 mm (178 du), When the touch ends, Then tool is unchanged, 0 nodes selected, 0 pan (palm-rest / tap); empty tap deselects."
  - "Given one finger on empty canvas with travel past 20 mm, When the finger moves, Then the local Epaper viewport pans and Infini's camera is unchanged unless Infini follow is on."
  - "Given three or more capacitive contacts on empty canvas, When any contact moves, Then 0 pan and 0 pinch."
  - "Given the hand-touch toggle off, When a finger lands on empty canvas or on a box, Then 0 canvas pick / pan / pinch and chrome taps still work."
  - "Given finger on a 64 du ToolChip tile, When tapped, Then REQ-03 still holds."
design_package: ".plan/iter-005/design/hand-touch/"
ui_spec: ".plan/iter-005/design/hand-touch/ui-spec.md"
scenes:
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-moving.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-finger-resizing.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-one-finger-empty-palm.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-one-finger-empty-pan.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-two-finger-pan.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pinch.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-pan-vs-move.html"
  - ".plan/iter-005/design/hand-touch/hand-touch-link-down-local-view.html"
hifi: ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
wireframe: ""
---

# STORY-EP-038 — One-finger hit box: select freeform and move

TRACK-005. Parent [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch). Depends on [STORY-EP-037](./STORY-EP-037.md) and [STORY-EP-054](./STORY-EP-054.md) (palm vs empty pan). [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md). Behavior-driven scenarios: [hand-touch-one-finger.feature](../../../.docs/modules/epaper/features/ink-box/bdd/hand-touch-one-finger.feature). **Done** 2026-08-20 (Quality Assurance Engineer verified host tests). **Human field-test approved** 2026-08-20: palm **20 mm / 178 du**, ≥3 contacts = palm, hand-touch toggle default on. Acceptance text restated to match Product Requirements Document 0.12.0-draft; status stays `done`.



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-037, STORY-EP-054 |

Behavior-driven scenarios authored 2026-08-20. Verified **done** 2026-08-20.
