---
id: STORY-EP-018
title: "On-device selection-create surround"
kind: implement
parent_srs: [SRS-EP-10, SRS-EP-11, SRS-EP-12, SRS-EP-14]
parent_req: [REQ-05]
status: done
priority: P1
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-012, STORY-EP-016, STORY-EP-022]
acceptance_criteria:
  - "Given tool.sel_rect and pen-down+move, When the gesture runs, Then ovl.marquee (thin dotted AABB) follows the pen tip; on pen-up, only inks with ≥80% of samples inside the rect are selected (grazing AABB does not select)."
  - "Given tool.sel_freeform, When the creator draws a polyline and pens up, Then ovl.lasso is a thin dotted polyline while drawing; pen-up closes it; membership is ≥80% of ink samples even-odd inside the polyline (not the gesture AABB); ovl.lasso is gone; ovl.nodes_bounds is the tight union AABB (0 pad) + 6 anchors + icon-only cta.enclose 64 du."
  - "Given pen-up after either mode, When the selection is non-empty, Then ovl.nodes_bounds tightly equals the union AABB of those document nodes (0 extra padding) with 6 square anchors, and cta.enclose is an icon-only 64 du control on SelectionOverlay with no context-toolbar chrome."
  - "Given ≥2 selected free inks where one stroke surrounds ≥80% of every other, When the user taps cta.enclose, Then the winner is role boundary, others are role content with layoutOffset UV, and bounds equal the winner AABB."
  - "Given an open surround stroke, When containment is tested, Then the artificial closed path (even-odd) may qualify and stored samples are unchanged."
  - "Given no qualifying surround or a SmartGroup in the selection, When cta.enclose is tapped, Then 0 boxes are created, selection is unchanged, and ind.create_refused_no_surround is visible."
  - "Given ToolChip, When Enclose is available, Then the chip has exactly four tools: sel_rect, sel_freeform, pen, ink_box (ADR-0017). Enclose is not a fifth chip."
design_package: ".plan/iter-003/design/selection-enclose-chrome/"
ui_spec: ".plan/iter-003/design/selection-enclose-chrome/ui-spec.md"
scenes:
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-marquee.html"
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-lasso.html"
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-nodes-selected.html"
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-create-refused.html"
hifi: ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-nodes-selected.html"
wireframe: ""
---

# STORY-EP-018 — On-device selection-create surround

Implements selection-create in [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md)
with chrome from [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) /
[ADR-0016](../../../.docs/adr/ADR-0016-selection-create-enclose-cta.md).
BDD: [selection-create-surround.feature](../../../.docs/modules/epaper/features/ink-box/bdd/selection-create-surround.feature)
(QA extends for marquee + Enclose CTA).

**Design done (EP-022).** Implement from [UI-EP-03](../design/selection-enclose-chrome/ui-spec.md).
Do **not** invent chrome. ToolChip is four tools (ADR-0017). Enclose is overlay-only.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-012, EP-016, **EP-022 (design)** |

## Done when

- `@SRS-EP-10` selection-create scenarios green (incl. marquee + CTA)
- UI matches EP-022 package; ToolChip is four tools; Enclose is not a chip
