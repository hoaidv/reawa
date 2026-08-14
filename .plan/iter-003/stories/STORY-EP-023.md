---
id: STORY-EP-023
title: "Rebase manipulation chrome onto four-tool chip"
kind: design
parent_srs: [SRS-EP-12]
parent_req: [REQ-06]
status: done
priority: P0
iter: iter-003
estimate: 2
owner: designer
depends_on: [STORY-EP-012, STORY-EP-022]
acceptance_criteria:
  - "Given ADR-0017, When UI-EP-02 scenes ship, Then ToolChip shows exactly sel_rect · sel_freeform · pen · ink_box on every manipulation state (sel.none, sel.selected, sel.moving, sel.resizing.*, sel.deselected, sel.unavailable, sel.reloaded)."
  - "Given tool.sel_rect or tool.sel_freeform armed, When a SmartGroup is selected, Then ovl.selection_bounds + ovl.resize_handles + tgl.ink_scale_mode appear; the armed selection tile stays invert; Enclose is not on the chip."
  - "Given UI-EP-03, When this package ships, Then it composes the four-tool chip from selection-enclose-chrome — 0 new tool glyphs, 0 third selection mode."
  - "Given platform epaper-device, When ui-spec-gate runs, Then it passes; ui_spec + scenes + hifi are copied onto STORY-EP-019."
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

# STORY-EP-023 — Rebase manipulation chrome onto four-tool chip

Updates [UI-EP-02](../design/device-selection-chrome/) so live manipulation (EP-019) matches
the four-tool chip shipped in [STORY-EP-022](./STORY-EP-022.md) / [ADR-0017](../../../.docs/adr/ADR-0017-four-tool-chip.md).

**Done 2026-08-14.** Chip composed from [UI-EP-03](../design/selection-enclose-chrome/). Handles 28/56 du and LOD 96 du unchanged.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | EP-012, EP-022 |
