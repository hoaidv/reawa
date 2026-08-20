---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, architect, qa, dev]
---

# Hand-off: Designer → Scrum Master — EP-056 pen-button map as Epaper

Story [STORY-EP-056](../stories/STORY-EP-056.md) is **done**. Package revised in place as **epaper-device**. Spec id **[UI-EP-08]** (does not keep UI-IN-03 as current).

## Package

| Field | Value |
|---|---|
| Spec | [UI-EP-08](../design/pen-button-map/ui-spec.md) Epaper on-device pen-button map editor |
| Path | `.plan/iter-005/design/pen-button-map/` |
| Platform | epaper-device · `data-platform: epaper` · 1-bit · no hover · navigator **80%** |
| Primary / hifi | `pen-button-map-layout-1.html` (`map.layout_1`) |
| States | `pen-button-map-states.html` |

Copied `ui_spec` / `scenes` / `hifi` onto [STORY-EP-052](../stories/STORY-EP-052.md) and [STORY-IN-035](../stories/STORY-IN-035.md). Did **not** edit `.docs/design/index.md` or `.docs/DESIGN.md` (SM stitches).

## Scenes

| File | State / journey |
|---|---|
| `pen-button-map-entry.html` | drawing + `cta.pen_map_open` |
| `pen-button-map-layout-0.html` | `map.layout_0` — 0 slot rows |
| `pen-button-map-layout-1.html` | `map.layout_1` — 1-button defaults |
| `pen-button-map-layout-2.html` | `map.layout_2` — 2-button defaults |
| `pen-button-map-offline.html` | `map.offline` — editor usable; persist waits |
| `pen-button-map-slot-click.html` | `scene.pen_map_click` — 3 Click items |
| `pen-button-map-slot-hold.html` | `scene.pen_map_hold` — 3 Hold-move items |
| `pen-button-map-chip-temp-erase.html` | SRS-EP-42 chip mirrors Temporary eraser |
| `pen-button-map-chip-drag.html` | SRS-EP-42 exclusive tool does not switch |
| `pen-button-map-states.html` | reactivity showcase |

Deleted Infini journeys: `pen-button-map-invalid-stale.html`, `pen-button-map-offline-then-publish.html`. Navigator no longer lists them.

## Proposed entry (`cta.pen_map_open`) — GAP-01

**Lone 10 mm 1-bit icon** in region `PenMapOpen`, orientation-top **leading**, sibling of ToolChip (same family as viewport-follow trailing). Glyph: stylus with barrel dots. **Not** Infini File menu, **not** a 5-way radio on exclusive tiles, **not** a fourth `toolMode`. PM adopts.

## Catalogues (closed)

- **Click:** Current primary ↔ Freeform Select · Current primary ↔ Eraser · Off. No Undo.
- **Hold-move:** Temporary eraser · Drag node under tip · Off. No temp freeform/rect.
- **Defaults:** 1-button Click = freeform toggle, Hold-move = Temporary eraser. 2-button B1 same; B2 Click = eraser toggle, Hold-move = Temporary eraser. 0-button: 0 rows.

## Gate

**ui-spec-gate: PASS** (human checklist) with campaign residuals below.

`adlc gate` design rows: CSS allowlist, tokens links, inlined navigator, interactivity — expected pass. Known mechanical noise:

- **Design platform** FAIL for `data-platform="epaper"` — [CHL-0002](../../iter-003/challenges/CHL-0002-epaper-platform-gate.md) adopted; engine patch pending human ADLC approval. Spec/SRS remain authoritative. Do not retarget to android/web.
- **srs-experience.md** missing for `epaper/tool-modes` — campaign override (do not hard-stop). Scene inventory = SRS-EP-52 + `srs-ui-multi-scene.md`. **Concern** — not silently edited into `.docs/modules/**`. No new `CHL-*`.

## Concerns

1. **Experience stub** — `tool-modes` has no `srs-experience.md`. Campaign override honored. PM may add later.
2. **GAP-01** — entry placement is a Designer proposal (leading 10 mm tile). Needs PM adopt.
3. **Design index** — still may list Infini UI-IN-03 until SM stitches `.docs/design/index.md`.
4. **CHL-0002** — mechanical `epaper` platform tag still outside `adlc` allowlist.
5. **STORY-IN-034** — historical Infini scene files `invalid-stale` / `offline-then-publish` were deleted so the navigator cannot show them as current. IN-034 stays `done`; its `scenes` list no longer points at those paths.

## Next

`/qa` BDD for REQ-18 / SRS-EP-43 / SRS-EP-52. Then `/dev` on EP-052 (dispatch) and IN-035 (persist/restore) — **not** until design done + BDD. Do not schedule an Infini map settings screen.
