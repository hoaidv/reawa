---
id: STORY-EP-019
title: "On-device live manipulation and REQ-08 conformance"
kind: implement
parent_srs: [SRS-EP-11, SRS-EP-14]
parent_req: [REQ-06]
status: draft
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-012, STORY-EP-016]
acceptance_criteria:
  - "Given the selected box's smaller on-panel axis is ≥ 96 du (not TILE_LOD_SCALE 0.35), When the pen presses a SmartGroup, Then the topmost sibling is selected and chrome appears p95 ≤100 ms."
  - "Given a press+drag inside bounds, When the gesture runs, Then the real ink follows the pen (≥5 Hz, stall ≤200 ms, 0 full-panel invalidations, 0 ghost) and release commits exactly one set_smart_transform whose geometry equals the last previewed geometry (0 px jump, 0 snap-back) — CHL-0006 / CHL-0007."
  - "Given fixedInk resize, When bounds change, Then each content ink keeps sample size (±1 px) and its own layoutOffset UV; boundary follows the frame; 0 unrelated content moves — CHL-0004 / CHL-0005."
  - "Given withBounds resize, When bounds change, Then content scales with the box and boundary always transforms with the frame."
  - "Given a handle dragged past the opposite edge, When released, Then width/height are non-negative and 0 negative-size state enters the document or the wire."
  - "Given empty-canvas press, When deselect runs, Then the next settled frame shows 0 residual selection pixels (CHL-0007)."
  - "Given scale below the LOD cutoff, When the pen presses a box, Then 0 transforms apply and ind.manipulation_unavailable is shown."
  - "Given the gesture router, When it resolves a SmartGroup press, Then verbs come from the capability descriptor {select, move, resize, set-ink-scale-mode}, 0 node-kind branches exist in the router, and the transform envelope carries an unset reserved rotation field."
  - "Given shared fixtures fixed-ink/, When device and desktop place content, Then they agree 100%."
design_package: ".plan/iter-003/design/device-selection-chrome/"
ui_spec: ".plan/iter-003/design/device-selection-chrome/ui-spec.md"
scenes:
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-none.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-selected.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-moving.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-resizing-with-bounds.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-resizing-fixed-ink.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-deselected.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-unavailable.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-reloaded.html"
hifi: ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-selected.html"
wireframe: ""
---

# STORY-EP-019 — On-device live manipulation and REQ-08 conformance

Implements [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) and the
manipulation + conformance bars of
[SRS-EP-14](../../../.docs/modules/epaper/features/ink-box/srs-quality.md).
BDD: [smart-group-selection.feature](../../../.docs/modules/epaper/features/ink-box/bdd/smart-group-selection.feature).

**Conformance stays in this story** (architect ask 5): a hard-coded `if SmartGroup` router
passes this iter's tests and costs [REQ-08](../../../.docs/modules/epaper/prd.md#node-manipulation)
its premise.

**Device units locked (architect 2026-08-13):** handle visual **28 du**, hit **56 du**, LOD =
smaller on-panel axis **< 96 du**. 1 du = 1 panel pixel @ 226 dpi. Do not fall back to 8 CSS px
or `TILE_LOD_SCALE 0.35`. Story stays `draft` until W11 (after EP-016). Chrome package is `done`.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-012 (design), EP-016 |

## Done when

- `@SRS-EP-11` scenarios green including the descriptor scenario
- `ui_spec` / `scenes` / `hifi` copied from EP-012
- CHL-0004…0007 named bars in SRS-EP-14 are 0
