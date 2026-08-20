---
from: pm
to: designer
date: 2026-08-20
iter: iter-005
cc: [sm, architect, qa]
---

# Hand-off: PM → Designer — pen-button map is epaper-device

Binding human 2026-08-20. SM will replan [STORY-IN-034](../stories/STORY-IN-034.md); **do not wait on PM to edit the story**. Revise the existing package — do not fork an Infini desktop screen.

## Contract

| Field | Value |
|---|---|
| Package | [`.plan/iter-005/design/pen-button-map/`](../design/pen-button-map/) |
| Platform | **epaper-device** (reMarkable 2 chrome). **1-bit. No hover.** |
| Parent REQ | [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) Configurable pen barrel-button accelerators |
| Not a parent | Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) — persist/restore **only**, `needs_design: no` |
| Product docs | epaper PRD **0.10.0-draft**; infini **0.7.0-draft** |

## Closed catalogues (Designer must not add items)

**Click** (discrete toggle):

- Current primary tool ↔ Freeform Select
- Current primary tool ↔ Eraser
- Off

**Hold-move** (temporary while button held **and** moving):

- Temporary eraser
- Drag node under tip
- Off

**Do not paint:** Undo on Click; `temp_sel_freeform`; `temp_sel_rect`; any other Click/Hold-move id. D9 lists are dead.

**Why no temp freeform on Hold-move:** Hold-move snaps back on release. Temporary freeform is meaningless if we do nothing after it and immediately switch back to the current tool. Same for temp rect. Lasting select is Click (current ↔ Freeform Select) or the ToolChip.

## Defaults to show

- 1-button: Click = current primary ↔ Freeform Select; Hold-move = **Temporary eraser**.
- 2-button: B1 as 1-button; B2 Click = current ↔ Eraser; B2 Hold-move = Temporary eraser.
- 0-button: **no** rows (0 fake bindings). Eraser **nib** is caption-only, not a slot.

## Journeys / states (dual-ask with `/qa`)

Must appear in Spec + grey-box (1-bit):

1. Map editor **off-canvas from Infini** — this is tablet chrome, not desktop settings.
2. **0-button** — slots absent.
3. **1-button** — one button, Click + Hold-move slots.
4. **2-button** — two buttons.
5. Click list open — exactly the three Click items.
6. Hold-move list open — exactly the three Hold-move items.
7. **Entry** into the editor from **device** chrome (propose the control; not an Infini File menu; not a 5-way radio on exclusive-tool tiles).
8. ToolChip **during hold-move Temporary eraser** — chip mirrors the temporary tool, then restores.
9. ToolChip **during hold-move Drag-under-tip** — exclusive tool on the chip **does not** switch.
10. Editor still usable when the desktop session is down (live map is on-device).

Drop as Infini journeys: offline-edit-then-publish on a desktop Save; hover/focus-ring slate chrome; invalid/stale **desktop** editor.

## Do not

- Paint Infini slate, desktop hover, Electron settings, or a Save-publish desktop footer as the map UI.
- Keep D9 four-item Click / five-item Hold-move lists.
- Put temp freeform or temp rect on Hold-move.
- Edit `.docs/` SRS, domain, ADR, `.docs/DESIGN.md`, or `.docs/design/index.md` (SM stitches index; architect owns SRS).
- Open REQ-08 / REQ-15 / CHL-0011 / CHL-0012 / EP-032 / AI.
- Add follow-toggle or hand-touch scenes to this package.

## Constraints

- HTML is a visual reference, not production Qt.
- Unique system files only if you add new icons (do not silently reuse Infini-only hover assets as the 1-bit truth).
- WIP 2: SM queues this lane; do not start if viewport-follow packages still occupy both slots.

## Out of scope

Infini persist/restore implementation. Wire fields. Catalogue **ids** (architect updates domain). Stories (SM).
