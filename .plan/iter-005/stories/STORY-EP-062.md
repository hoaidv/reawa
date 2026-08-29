---
id: STORY-EP-062
title: Eraser mode, ToolChip, barrel last-used
kind: implement
parent_srs: [SRS-EP-54, SRS-EP-59]
parent_req: [REQ-11, REQ-03, REQ-18, REQ-10, REQ-20]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given the ToolChip, When shown, Then HT is a toggle in the first cluster, then sel_rect / sel_freeform / pen, gap, two recognizers, gap, erase_brush / erase_area / erase_object, gap, Undo/Redo; 0 second HT tile on the trailing row."
  - "Given any eraser armed, When the chip is shown, Then both recognizers are dimmed (armed latch kept) and an erase stroke never runs enclose or connector recognition."
  - "Given last-used = erase_area, When barrel Click toggle_pen_eraser from pen, Then exclusive tool becomes erase_area (p95 <=300 ms chip indicator)."
  - "Given pen and Hold-move temp_erase, When hold-move, Then last-used eraser runs until release and the chip mirrors it; 0 click toggle on that release."
  - "Given an eraser already armed, When Hold-move temp_erase, Then 0 tool change (no-op)."
  - "Given last-used default, When the app launches on this device, Then last-used is erase_brush until another eraser is armed; the value persists on-device (REQ-20), not in SVG."
  - "Given inverted nib (if HID reports it), When inverted, Then mutation is brush and the chip shows erase_brush; on un-invert, exclusive tool restores."
  - "Given empty erase gesture (down+up, no work), When commit, Then 0 tree ops and 0 undo entries."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-062 — Eraser mode, ToolChip, barrel last-used

Chip + `Eraser` InteractionMode + routing + barrel/nib. **No document clip yet.** Human waived a design package; three 1-bit glyphs are already in `.docs/design/system/assets/` (`icon-epaper-erase-brush.svg`, `icon-epaper-erase-area.svg`, `icon-epaper-erase-object.svg`). Rasterize into `epaper/icons/` the same way as pen / sel-freeform.

Canonical product: [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md) §§2–4, 12, 14 Chip/barrel. Bind: [SRS-EP-54](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-54-erase-mode). Mode pattern: [ADR-0033](../../../.docs/adr/ADR-0033-tool-abstraction.md) (one Mode, three Operations). Barrel catalogues: [pen-button-map](../../../.docs/domain/pen-button-map.md).

Human is QA this wave: host tests + human confirm. No BDD ceremony required before implement.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — (icons already delivered; [STORY-EP-040](./STORY-EP-040.md) cancelled) |

## Done when

- Six exclusives + HT-on-chip; trailing HT tile gone
- Recognizers dim under any eraser
- Last-used persist; Click / Hold-move / nib rules from SRS-EP-54
- Secondary in eraser modes = Navigation only
- Quality: chip arm p95 ≤300 ms ([SRS-EP-59](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-59-erase-quality))
