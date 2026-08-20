---
id: CHL-0025
author: designer
target: [SRS-EP-52]
severity: medium
status: resolved
resolution: adopted
opened: 2026-08-20
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0025 — Pen-button map is one settings page (master-detail)

## Context

Human 2026-08-20, after visual review of [UI-EP-08](../design/pen-button-map/):

> should merge map.layout vs map.slot_click, map.slot_hold into 1 page setting. This is simple and small.
> Build a setting page, using master-detail layout, and this pen-button map is the first master menu of the setting page.

This overrides [SRS-EP-52](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) `present-sheet` lists and the three-scene graph in [srs-ui-multi-scene.md](../../../.docs/modules/epaper/features/tool-modes/srs-ui-multi-scene.md) (`scene.pen_map_editor` + `scene.pen_map_click` + `scene.pen_map_hold`).

Designer does **not** edit product docs. Package [UI-EP-08](../design/pen-button-map/ui-spec.md) follows the human override now.

## Proposal

| Topic | Adopt |
|---|---|
| Shell | Full-panel **Settings** page, **master-detail**. Master: first item **Pen buttons**. No other settings items this package (do not invent inventory). |
| Catalogues | Click and Hold-move lists live **in the detail pane** (inline radios). No sheet scenes. |
| Capability | 0 / 1 / 2 / offline remain **states of the same page**, not separate products. |
| Entry | `cta.pen_map_open` still opens this page (`present-modal` → settings). Close dismisses to drawing. |
| Drop | `present-sheet` hops; `map.slot_click` / `map.slot_hold` as their own HTML scenes; list cancel (write is in-place). |

Chip hold-move journeys ([SRS-EP-42](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool)) stay drawing scenes — not the settings page.

## Resolution

**Adopted** 2026-08-20 (pm). Human override + Designer paint of [UI-EP-08](../design/pen-button-map/ui-spec.md). Same TRACK-005 slice; not an interrupt.

Also adopted in the same product pass (human lock 2026-08-20):

| Topic | Decision |
|---|---|
| Persist home | **Epaper device**, not Infini, not the document. No document settings. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore **retired** (superseded-by [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings)). |
| GAP-01 | **Adopt.** Leading 10 mm stylus-with-barrels tile (`cta.pen_map_open`) is a sibling of ToolChip — not a fourth exclusive tool. Written into [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings). |

## Product doc updates

- `.docs/modules/epaper/prd.md` — minted [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) Device Settings; amended [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) (inline catalogues; persist is REQ-20); thickened [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) pan threshold = **10 mm**
- `.docs/modules/infini/prd.md` — retired [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map)
- `.docs/modules/epaper/features/device-document/srs-product.md` — BR-D07 does not apply to Device Settings
- `.docs/modules/epaper/features/tool-modes/index.md` — `parent_req` includes REQ-20

Architect **must** rebind (PM did not edit): `srs-ui.md`, `srs-ui-multi-scene.md`, `srs-logic.md`, `srs-quality.md`, Infini `SRS-IN-23` / `SRS-IN-25`, [ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md), domain `pen-button-map.md`.

## Interrupt / expedite (when applicable)

Not an interrupt — same TRACK-005 / REQ-18 slice.
