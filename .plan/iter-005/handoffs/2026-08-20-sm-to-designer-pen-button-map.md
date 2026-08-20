---
from: sm
to: designer
date: 2026-08-20
iter: iter-005
cc: [pm, architect, qa]
---

# Hand-off: Scrum Master → Product Designer — EP-056 pen-button map as Epaper

Story [STORY-EP-056](../stories/STORY-EP-056.md) is **ready**. Architecture bound. Revise the **existing** package — do not fork an Infini desktop screen.

## Story / parents

| Field | Value |
|---|---|
| Story | [STORY-EP-056](../stories/STORY-EP-056.md) Revise pen-button map as Epaper on-device editor |
| Package | `.plan/iter-005/design/pen-button-map/` |
| Parent REQ | [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) |
| Parent SRS | [SRS-EP-52](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) · [SRS-EP-53](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author) |
| Scene graph | `.docs/modules/epaper/features/tool-modes/srs-ui-multi-scene.md` |
| ADR | [ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) (supersedes ADR-0028 authoring) |
| Not a parent | Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) / retired [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md) |

Also read [pm-to-designer-pen-button-map](./2026-08-20-pm-to-designer-pen-button-map.md) and [architect-to-sm-pen-button-map](./2026-08-20-architect-to-sm-pen-button-map.md).

## Binding catalogues (do not add items)

**Click:** Current primary ↔ Freeform Select · Current primary ↔ Eraser · Off. No Undo.

**Hold-move:** Temporary eraser · Drag node under tip · Off. **No** temporary freeform or temp rect (Hold-move snaps back; temp freeform is meaningless).

**Defaults:** 1-button Click = current ↔ Freeform Select; Hold-move = Temporary eraser. 2-button: B1 as 1-button; B2 Click = current ↔ Eraser; B2 Hold-move = Temporary eraser. 0-button: 0 rows.

## Platform

**epaper-device.** 1404×1872, 1-bit, no hover, no focus, no cursor, no motion. Finger hits ≥ 64 du. `data-platform: epaper`. Historical IN-034 Infini slate / hover / Save-publish is **not Keep**.

## Scene ids (Software Requirements Specification bind)

- `scene.pen_map_editor` — hub (`map.layout_0` / `_1` / `_2` / `map.offline`)
- `scene.pen_map_click` — Click list (exactly three items)
- `scene.pen_map_hold` — Hold-move list (exactly three items)

Entry: propose `cta.pen_map_open` placement. Must not be Infini File menu, a 5-way radio on exclusive-tool tiles, or a fourth exclusive tool.

**REQ-18 also dual-asks chip during hold-move** ([SRS-EP-42](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool)): Temporary eraser mirrors then restores; Drag-under-tip does **not** switch exclusive tool. Put those as **additional scenes in this package** if they fit; do **not** bury them in hand-touch or viewport-follow packages. They are **not** Infini screens.

## Do not

- Edit `.docs/` Software Requirements Specification, domain, Architecture Decision Records, Product Requirements Documents, `.docs/DESIGN.md`, or `.docs/design/index.md` (Scrum Master stitches the index).
- Keep D9 four-item Click / five-item Hold-move lists.
- Paint Infini desktop settings chrome.
- Open REQ-08 / REQ-15 / CHL-0011 / CHL-0012 / EP-032 / AI.
- Edit `hand-touch/` or viewport-follow packages.

On gate pass: set STORY-EP-056 `done` with `ui_spec` / `scenes` / `hifi`. Copy those onto EP-052 and IN-035. Handoff `.plan/iter-005/handoffs/2026-08-20-designer-to-sm-pen-button-map.md`. New Spec id should be an **Epaper** user-interface id (do not keep UI-IN-03 as current).
