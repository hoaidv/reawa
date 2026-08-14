---
id: STORY-EP-025
title: "Selection chrome CanvasLayer / ToolCanvasLayer / ToolLayer"
kind: implement
parent_srs: [SRS-EP-12, SRS-EP-14]
parent_req: [REQ-06]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-EP-018, STORY-EP-022, STORY-EP-023]
acceptance_criteria:
  - "Given tool.sel_freeform and a lasso in flight, When the pen moves, Then ovl.lasso is painted on ToolCanvasLayer (not CanvasLayer m_image), ToolCanvas waveform is Pen, damage is the last segment AABB, and CanvasLayer is not update()'d with an empty rect — 0 full-panel invalidations (CHL-0017 / ADR-0019 / SRS-EP-14)."
  - "Given tool.sel_rect and a marquee in flight, When the pen moves, Then ovl.marquee follows the tip on ToolCanvasLayer with Pen waveform and old∪new AABB damage only."
  - "Given pen-up after either gesture, When chrome settles (AABB / move / resize), Then ToolCanvas waveform is Mono; ovl.lasso is gone; ovl.nodes_bounds is the tight union AABB on ToolCanvasLayer."
  - "Given tool.pen or tool.ink_box, When the pen inks, Then CanvasLayer Pen path is unchanged (p95 ink budget not regressed) and ToolCanvasLayer does not steal samples."
  - "Given enclose / membership / set_smart_transform, When this story ships, Then those code paths are unchanged (0 SmartGroup logic edits except chrome subscription to selectedIds + AABB)."
design_package: ".plan/iter-003/design/selection-enclose-chrome/"
ui_spec: ".plan/iter-003/design/selection-enclose-chrome/ui-spec.md"
scenes:
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-marquee.html"
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-lasso.html"
  - ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-nodes-selected.html"
hifi: ".plan/iter-003/design/selection-enclose-chrome/selection-enclose-chrome-sel-lasso.html"
wireframe: ""
---

# STORY-EP-025 — Selection chrome CanvasLayer / ToolCanvasLayer / ToolLayer

Implements the refresh-class split in
[ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md) /
[CHL-0017](../challenges/CHL-0017-selection-chrome-layers.md) for
[SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md).

**No new design package.** Visual inventory is UI-EP-03 (lasso/marquee/settled) + UI-EP-02
(handles / mode chip). This story is composition, damage, and waveform.

**Do not start while EP-019 is in flight** — both write `epaper/tabletcanvasitem.cpp`. W11c
after W11b verify. **Do not** change surround-create or manipulation ops.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-018, EP-022, EP-023 (design already `done`) |

## Done when

- Lasso/marquee in flight uses ToolCanvas **Pen**; pen-up settled + move/resize uses **Mono**
- Host tests for EP-018 / EP-019 still PASS
- Human PASS 2026-08-14 (option 1 live node on ToolCanvas; mid-gesture ghosting allowed — CHL-0018)
