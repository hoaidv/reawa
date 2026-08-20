---
from: pm
to: sm
date: 2026-08-20
iter: iter-005
cc: [architect, designer, qa]
---

# Hand-off: PM → SM — replan pen-button map (Epaper, not Infini)

PM does **not** create or edit stories. Product docs updated. Replan after this handoff; architect binds SRS (listed below — do not wait on PM).

## Verdict

**READY-WITH-CONCERNS.** Human 2026-08-20 is binding.

`adlc prd-check`: **0 FAIL**. Infini: no findings. Epaper WARNs are pre-existing closed Open Questions (no `owner` on the first bullet line). `reawa` WARNs are out of lock.

PRDs: epaper **0.10.0-draft**, infini **0.7.0-draft**.

### Concerns (accepted — not PRD gaps)

- Domain / SRS-IN-24 / ADR-0028 still say Infini authors a desktop editor. Architect rebinds (listed below). PM did not edit those.
- [STORY-IN-034](../stories/STORY-IN-034.md) is `done` as Infini slate — that paint is not the product. Revise the same package as epaper-device.
- On-device **entry** control is unnamed (designer proposes; PM will adopt). Do not invent Infini File menu.

## What changed in product

The design package [`.plan/iter-005/design/pen-button-map/`](../design/pen-button-map/) is **Epaper**, not Infini. The creator configures barrel buttons **on the tablet** (epaper-device chrome). Infini desktop settings as the map **UI** is wrong.

| ID | Title | Fate |
|---|---|---|
| [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) | Configurable pen barrel-button accelerators | **Amended.** `needs_design: yes` **on Epaper**. UI = on-device map editor (0/1/2 buttons, closed lists). Chip still mirrors temporary tool during hold-move (Temporary eraser). |
| [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) | Pen-button map persist (not the editor) | **Re-scoped.** Id kept (`active`). Desktop editor **UI outcome retired in place**. Persist/restore remains Must. `needs_design: **no**`. Not a settings surface. |

Closed catalogues (replace D9):

**Click:** Current primary tool ↔ Freeform Select · Current primary tool ↔ Eraser · Off. (No Undo.)

**Hold-move:** Temporary eraser · Drag node under tip · Off. **Removed** `temp_sel_freeform` and `temp_sel_rect` — Hold-move snaps back; temp freeform is meaningless if we do nothing after it.

**Defaults:** 1-button Click stays current-tool ↔ Freeform Select. 1-button Hold-move = **Temporary eraser** (was temp freeform). 2-button: B1 as 1-button; B2 Click = current ↔ Eraser; B2 Hold-move = Temporary eraser.

## Must replan (existing)

| Story | Why |
|---|---|
| [STORY-IN-034](../stories/STORY-IN-034.md) | Title/parent are Infini desktop settings. **Package is Epaper.** `parent_req` should be [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons). Designer **revises** `pen-button-map/` as **epaper-device** (1-bit, no hover). Status `done` as Infini slate is **not** current — follow-on revise (same package) or reopen per SM practice. Do **not** delete the story id. |
| [STORY-IN-035](../stories/STORY-IN-035.md) | Persist/publish may remain under Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map), but drop “assign slots and save on desktop” / settings-UI AC. Freeze until architect rebinds **direction** (tablet authors; Infini persist/restore). Do not implement a desktop editor. |
| [STORY-EP-052](../stories/STORY-EP-052.md) | AC still says default Hold-move = temporary freeform. Replan to Temporary eraser + new catalogues. Parent stays REQ-18. `depends_on` IN-034 only after that story is the **epaper-device** package. |

Do **not** bury this in viewport-follow or hand-touch packages.

## Architect follow-up (SM dispatch; PM did not write SRS / domain / ADR)

1. [domain/pen-button-map.md](../../../.docs/domain/pen-button-map.md) catalogues + defaults (drop `undo`, `temp_sel_freeform`, `temp_sel_rect`; Hold-move default `temp_erase`; Infini is persist, not author of the editor).
2. [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md) pen-button map settings — **rehome or retire** (lifecycle in place; do not delete the id). Not a desktop editor.
3. New **epaper** `srs-ui` for the on-device map editor if needed (chip mirror [SRS-EP-42](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md) is not the editor).
4. [ADR-0028](../../../.docs/adr/ADR-0028-pen-button-map-settings-channel.md) currently assumes Infini authors and publishes down — Architect rebinds tablet-author / Infini persist-restore. PM does not write the ADR.

## Dual-ask

On-device editor states in [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) AC: `/designer` Spec + scenes (see [pm-to-designer](./2026-08-20-pm-to-designer-pen-button-map.md)); `/qa` BDD after SM confirms the board.

## Do not

- Create or edit stories in this PM turn (you are SM).
- Treat IN-034’s Infini desktop paint as the shipping UI.
- Schedule an Infini slate/desktop map settings screen.
- Reopen REQ-08 / REQ-15 / CHL-0011 / CHL-0012 / EP-032 / AI.
- Open this package while WIP 2 is full (viewport-follow lanes may still occupy). Queue; do not bury.

## Next

`/architect` on catalogues + SRS-IN-24 rehome/retire + epaper srs-ui if needed. Then `/sm` replan IN-034 parent_req + designer revise. Then `/designer` per the designer handoff. `/qa` after the revised package.
