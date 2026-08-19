---
from: designer
to: sm
date: 2026-08-19
iter: iter-005
---

# Hand-off: Designer → SM — IN-034 done

## Context

Lane B painted Infini **pen-button map settings** as `[UI-IN-03]`. Package: [`.plan/iter-005/design/pen-button-map/`](../design/pen-button-map/). Story [STORY-IN-034](../stories/STORY-IN-034.md) is **done**. Primary scene: `pen-button-map-layout-1.html`. Open `index.html` (iframe navigator, desktop @ 100%).

Copied `design_package` / `ui_spec` / `scenes` / `hifi` onto [STORY-IN-035](../stories/STORY-IN-035.md) and [STORY-EP-052](../stories/STORY-EP-052.md). EP-052 acceptance was **not** changed. Did **not** edit `.docs/DESIGN.md`, `.docs/design/index.md`, or any SRS.

Catalogues are closed: Click discrete-only (`toggle_pen_freeform` · `toggle_pen_eraser` · `undo` · `off`); Hold-move temporary-tool-only (`temp_sel_freeform` · `temp_sel_rect` · `temp_erase` · `drag_node_under_tip` · `off`). 0-button has **no** rows (0 fake bindings). Eraser nib is caption-only, not a slot ([ADR-0025](../../../.docs/adr/ADR-0025-barrel-vs-eraser-nib.md)). Save is settings publish, not document chrome ([ADR-0028](../../../.docs/adr/ADR-0028-pen-button-map-settings-channel.md)).

## Asks

1. `/sm` stitch `.docs/design/index.md` row for `[UI-IN-03]` after Lane A joins (lock: Designer did not edit the index).
2. `/pm` optional thicken: `srs-experience`, copy table, interaction-map feedback, entry control from Infini chrome (see Concerns).
3. `/qa` BDD then `/dev` on IN-035 — **after** design done + BDD (lock). Do not `/dev` yet.

## Concerns (experience not thickened — campaign override)

- No `srs-experience.md` / `srs-ui-multi-scene.md` for tablet-sync. Scene inventory = SRS-IN-24 state ids only. Did not invent a third slot type or a tablet 5-way radio.
- SRS-IN-24 is a skeleton (no copy table, no feedback-column interaction map, no control-states block). Those were drafted **in the UI Spec** for PM adopt — product docs were not silently edited.
- No entry CTA from canvas/menu is specified. This package **is** the settings surface (not a modal), so no invented File menu.

## Constraints

- HTML is a visual reference, not production Electron/React.
- Infini slate/ink; hover required; `data-platform: desktop`.
- Unique system files only: `icon-pen-map-*.svg`, `pen-map-button.html`, `pen-map-select.html`.

## Out of scope

REQ-15 tables · REQ-08 · CHL-0011 · CHL-0012 · EP-032 · AI · EP-037 / `design/hand-touch/`.
