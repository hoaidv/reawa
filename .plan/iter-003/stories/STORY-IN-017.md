---
id: STORY-IN-017
title: "Selection create requires surround stroke"
kind: implement
parent_srs: [SRS-IN-16]
parent_req: [REQ-04]
status: draft
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-IN-012, STORY-IN-013, STORY-IN-014]
acceptance_criteria:
  - "Given ≥2 selected inks where one stroke surrounds ≥80% of every other (artificial close + even-odd PIP if open), When create Smart Group runs, Then winner is boundary, others content with layoutOffset UV, bounds = winner AABB."
  - "Given a selection with no qualifying surround, When create runs, Then create is refused; selection unchanged; refuse UI per Spec."
  - "Given several qualifying surrounds, When create runs, Then the later sibling wins."
  - "Given success, When undo runs, Then the prior snapshot restores."
design_package: ".plan/iter-003/design/ink-box-ui/"
ui_spec: ".plan/iter-003/design/ink-box-ui/ui-spec.md"
scenes:
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-create-refused.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-selected.html"
hifi: ".plan/iter-003/design/ink-box-ui/ink-box-ui-create-refused.html"
wireframe: ""
---

# STORY-IN-017 — Selection create with surround guard

Implements [SRS-IN-16](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-16-selection-create-surround).
Refuse UI from [STORY-IN-013](./STORY-IN-013.md) — Spec [UI-IN-02](../design/ink-box-ui/ui-spec.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | IN-012, IN-013 (design), IN-014 |

## Done when

- AC green `@SRS-IN-16`
