---
id: STORY-IN-015
title: "Smart Group selection hit-test move resize and fixedInk UV"
kind: implement
parent_srs: [SRS-IN-11]
parent_req: [REQ-04]
status: draft
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-IN-012, STORY-IN-013, STORY-IN-014]
acceptance_criteria:
  - "Given scale ≥ TILE_LOD_SCALE and a SmartGroup, When the pointer hits world bounds, Then the node selects (topmost sibling first) and shows overlay per design Spec."
  - "Given a selected SmartGroup, When the user drags inside bounds, Then set_smart_transform translate applies on release (one op) and the canvas does not pan."
  - "Given fixedInk and content layoutOffset {u,v}, When bounds resize, Then each content ink keeps sample size and its UV (±1 px @ 100%); smartLocalToWorld must use UV (not translate-only)."
  - "Given withBounds, When bounds resize, Then content samples scale with the group; boundary ink always transforms."
  - "Given scale < TILE_LOD_SCALE, When pressing a pickable, Then picking is disabled and pan wins."
design_package: ".plan/iter-003/design/ink-box-ui/"
ui_spec: ".plan/iter-003/design/ink-box-ui/ui-spec.md"
scenes:
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-idle.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-selected.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-dragging.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-ink-box-armed.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-manipulation-unavailable.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-create-refused.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-states.html"
hifi: ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-idle.html"
wireframe: ""
---

# STORY-IN-015 — Selection, hit-test, move/resize, fixedInk UV

Implements [SRS-IN-11](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-11-selection-manipulation).
UI from [STORY-IN-013](./STORY-IN-013.md) — Spec [UI-IN-02](../design/ink-box-ui/ui-spec.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | IN-012, IN-013 (design), IN-014 |

## Done when

- AC green `@SRS-IN-11`; Spec paths copied after design done; status → ready only then
