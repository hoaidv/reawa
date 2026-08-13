---
id: STORY-EP-012
title: "Design device selection overlay and manipulation chrome"
kind: design
parent_srs: [SRS-EP-12]
parent_req: [REQ-05, REQ-06]
status: done
priority: P0
iter: iter-003
estimate: 5
owner: designer
depends_on: []
acceptance_criteria:
  - "Given SRS-EP-12, When the package ships, Then SelectionOverlay (bounds, resize handles, inkScaleMode toggle, mode indicator) is specified for epaper-device 1872×1404, 1-bit, pen input, no hover, no motion."
  - "Given the states matrix, When scenes ship, Then each of sel.none, sel.selected, sel.moving, sel.resizing.with_bounds, sel.resizing.fixed_ink, sel.deselected, sel.create_refused, sel.unavailable, sel.reloaded is a package-contained scene with a trace to the SRS state id."
  - "Given live manipulation, When chrome is drawn, Then the overlay annotates ink that is already moving — 0 ghost, marquee, or stand-in scenes (do not port epaper-tool-strip selection-dragging ghost)."
  - "Given hardware spike, When the Spec records device constants, Then handle size and hit tolerance are in device units (not 8 CSS px) and the LOD cutoff is proposed in panel terms (not TILE_LOD_SCALE 0.35) — both flagged open until architect confirms."
  - "Given scarce chrome, When undo and selection-create are considered, Then the Spec answers whether an undo affordance fits the three-tool chip and how selection-create is invoked — or files a CHL if they cannot share the surface."
  - "Given 1-bit dense handwriting, When chrome sits on ink, Then handles and bounds remain distinguishable from strokes without tint or shadow."
  - "Given ui-spec-gate, When run, Then hi-fi scenes + Spec pass; ToolChip is composed from EP-003, not redesigned; no rotation handle, no properties panel."
design_package: ".plan/iter-003/design/device-selection-chrome/"
ui_spec: ".plan/iter-003/design/device-selection-chrome/ui-spec.md"
scenes:
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-none.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-selected.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-moving.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-resizing-with-bounds.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-resizing-fixed-ink.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-deselected.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-create-refused.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-unavailable.html"
  - ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-reloaded.html"
hifi: ".plan/iter-003/design/device-selection-chrome/device-selection-chrome-sel-selected.html"
wireframe: ""
---

# STORY-EP-012 — Design device selection overlay and manipulation chrome

Designs [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) for
[REQ-05](../../../.docs/modules/epaper/prd.md#device-ink-box) /
[REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation).
Journeys: [srs-experience](../../../.docs/modules/epaper/features/ink-box/srs-experience.md).
Extends [STORY-EP-003](./STORY-EP-003.md) ToolChip; new package, not a port of deprecated
[ink-box-ui](../design/ink-box-ui/).

**Spike inside this story** (architect ask 1): handle size, LOD cutoff, undo affordance,
selection-create invocation. Do not leave those to an implement story.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

## Done when

- `ui-spec-gate` pass; `ui_spec` + `scenes` + `hifi` set
- Open questions in SRS-EP-12 either answered in the Spec or escalated via `CHL-*`
- [STORY-EP-019](./STORY-EP-019.md) can copy Spec/scenes; [STORY-EP-018](./STORY-EP-018.md) has an invocation answer
