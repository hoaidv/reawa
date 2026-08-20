---
id: STORY-EP-054
title: Revise hand-touch: palm-rest vs empty local pan
kind: design
parent_srs: [SRS-EP-21, SRS-EP-22, SRS-EP-23, SRS-EP-24]
parent_req: [REQ-10]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: [STORY-EP-037]
acceptance_criteria:
  - "Given one-finger empty canvas travel at/below 20 mm (178 du), When shown, Then the scene is palm-rest / tap with 0 pan."
  - "Given one-finger empty canvas travel past 20 mm, When shown, Then the scene is local pan (Infini camera unchanged unless Infini is following)."
  - "Given two-finger pan, When shown, Then Infini match is not implied unless Infini follow is on."
  - "Given the trailing HT tile, When shown, Then it is a 64 du kill-switch left of Debug (inverted when on), not a pan-mode / hand-tool tile."
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

# STORY-EP-054 — Revise hand-touch: palm-rest vs empty local pan

Follow-on to [STORY-EP-037](./STORY-EP-037.md) (`done`). Same package `hand-touch/`. **Do not** add follow-toggle buttons here ([STORY-EP-053](./STORY-EP-053.md)). Product [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) names the threshold (human field-test lock 2026-08-20, supersedes the earlier 10 mm / 89 du lock): **≤ 20 mm** palm-rest / tap (0 pan); **> 20 mm** local pan; **≥3** contacts = palm. Architect millimetre↔du bind is **178 du @ 226 dpi**. `btn.hand_touch` (HT) is a 64 du kill-switch left of Debug — not a pan-mode tile.

Replace `hand.one_finger_empty` no-op-only with `hand.one_finger_empty_palm` (≤20 mm / 178 du) and `hand.one_finger_empty_pan` (>20 mm). Box / knob / chip hit still wins. **Human field-test approved** 2026-08-20; package [UI-EP-06](../design/hand-touch/ui-spec.md) amended in place; status stays `done`.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | STORY-EP-037 |
