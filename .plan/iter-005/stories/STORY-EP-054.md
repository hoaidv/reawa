---
id: STORY-EP-054
title: Revise hand-touch: palm-rest vs empty local pan
kind: design
parent_srs: [SRS-EP-21, SRS-EP-22, SRS-EP-23, SRS-EP-24]
parent_req: [REQ-10]
status: ready
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: [STORY-EP-037]
acceptance_criteria:
  - "Given one-finger empty canvas travel at/below 10 mm, When shown, Then the scene is palm-rest / tap with 0 pan."
  - "Given one-finger empty canvas travel past 10 mm, When shown, Then the scene is local pan (Infini camera unchanged unless Infini is following)."
  - "Given two-finger pan, When shown, Then Infini match is not implied unless Infini follow is on."
design_package: ".plan/iter-005/design/hand-touch/"
ui_spec: ".plan/iter-005/design/hand-touch/ui-spec.md"
scenes: []
hifi: ".plan/iter-005/design/hand-touch/hand-touch-finger-hit-box.html"
wireframe: ""
---

# STORY-EP-054 — Revise hand-touch: palm-rest vs empty local pan

Follow-on to [STORY-EP-037](./STORY-EP-037.md) (`done`). Same package `hand-touch/`. **Do not** add follow-toggle buttons here ([STORY-EP-053](./STORY-EP-053.md)).

Replace `hand.one_finger_empty` no-op-only with `hand.one_finger_empty_palm` (≤10 mm / 89 du) and `hand.one_finger_empty_pan` (>10 mm). Box / knob / chip hit still wins.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | STORY-EP-037 |
