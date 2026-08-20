---
from: sm
to: architect
date: 2026-08-20
iter: iter-005
cc: [pm, designer]
---

# Hand-off: Scrum Master → Solution Architect — pen-button map catalogues + home

Product Manager already amended the Product Requirements Documents (epaper **0.10.0-draft**, infini **0.7.0-draft**). Human 2026-08-20 is binding. Do **not** wait on Product Manager.

## Product truth (do not soften)

- Map **editor** is **on-device Epaper** ([REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons)). Package `.plan/iter-005/design/pen-button-map/` is Epaper, not Infini.
- Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map): persist/restore only. Desktop editor user-interface outcome **retired in place**. Id kept.
- Click (closed): `toggle_pen_freeform` · `toggle_pen_eraser` · `off`. **Drop `undo`.**
- Hold-move (closed): `temp_erase` · `drag_node_under_tip` · `off`. **Drop `temp_sel_freeform` and `temp_sel_rect`.**
- Rationale: Hold-move snaps back; temporary freeform is meaningless if we do nothing after it.
- Defaults: 1-button Click `toggle_pen_freeform`; Hold-move **`temp_erase`**. 2-button: B1 as 1-button; B2 Click `toggle_pen_eraser`; B2 Hold-move `temp_erase`.

## Must write

1. [domain/pen-button-map.md](../../../.docs/domain/pen-button-map.md) — catalogues, defaults, author is tablet; Infini persist/restore. Drop stale `map.stale` “Infini edited offline” if authoring is on-device.
2. New **epaper** Software Requirements Specification for the on-device map editor (user-interface + logic as needed). Chip mirror stays [SRS-EP-42](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md); that is **not** the editor.
3. [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md) — **retire in place** (do not delete the id). Not a desktop editor.
4. Rebind [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) / [SRS-IN-25](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md) to persist/restore **up** (tablet authors → Infini stores).
5. [ADR-0028](../../../.docs/adr/ADR-0028-pen-button-map-settings-channel.md) — do not overwrite. Amend or supersede: direction reverses (tablet authors; Infini persist/restore). Settings channel (0 document messages) can stay.

## Do not

- Edit Product Requirements Documents, stories, design HTML, Master Plan, `src/`.
- Reopen requirement 15 / 08, Challenge 0011 / 0012, EP-032, AI.
- Keep Infini as author of the map editor.

## Stories the Scrum Master already replanned

- [STORY-EP-056](../stories/STORY-EP-056.md) — design revise package as epaper-device. `parent_srs` nearest [SRS-EP-41] / [SRS-EP-42] until you bind.
- [STORY-IN-034](../stories/STORY-IN-034.md) — historical Infini paint; id kept `done`.
- [STORY-IN-035](../stories/STORY-IN-035.md) / [STORY-EP-052](../stories/STORY-EP-052.md) — wait on your bind.

Handoff back: `.plan/iter-005/handoffs/2026-08-20-architect-to-sm-pen-button-map.md`.
