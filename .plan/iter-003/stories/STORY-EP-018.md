---
id: STORY-EP-018
title: "On-device selection-create surround"
kind: implement
parent_srs: [SRS-EP-10, SRS-EP-14]
parent_req: [REQ-05]
status: draft
priority: P1
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-EP-012, STORY-EP-016]
acceptance_criteria:
  - "Given ≥2 selected inks where one stroke surrounds ≥80% of every other, When create Smart Group runs on the device, Then the winner is role boundary, others are role content with layoutOffset UV, and bounds equal the winner AABB."
  - "Given an open surround stroke, When containment is tested, Then the artificial closed path (even-odd) may qualify and stored samples are unchanged."
  - "Given no qualifying surround, When create runs, Then 0 boxes are created, selection is unchanged, and the refuse reason is visible per the EP-012 Spec (`ind.create_refused_no_surround`)."
  - "Given the EP-012 invocation answer, When the story is implemented, Then create is triggered by that gesture or control — do not invent a second product."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-018 — On-device selection-create surround

Implements selection-create in [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md).
BDD: [selection-create-surround.feature](../../../.docs/modules/epaper/features/ink-box/bdd/selection-create-surround.feature).

**Waits on [STORY-EP-012](./STORY-EP-012.md)** for how create is invoked and how refuse is shown.
Logic already exists in SRS; chrome does not.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-012 (design), EP-016 |

## Done when

- `@SRS-EP-10` selection-create scenarios green
- `ui_spec` / `scenes` copied from EP-012 for the refuse state
