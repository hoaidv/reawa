---
from: architect
to: sm
date: 2026-08-20
iter: iter-005
verdict: READY-WITH-CONCERNS
cc: [pm, designer, qa]
---

# Hand-off: Architect → SM — pen-button map catalogues + home

Stories, MASTER, board, design HTML, PRDs, and `src/` were **not** edited. Replan from this bind + [sm-to-architect-pen-button-map](./2026-08-20-sm-to-architect-pen-button-map.md).

## Verdict

**READY-WITH-CONCERNS.** Closed catalogues, tablet-as-author, and Infini persist/restore are testable. The on-device editor has UI + logic + a 3-scene graph. Desktop editor UI is retired in place. Concerns below are placement/package-delta follow-ups, not missing Must specs.

No `CHL-*`. ADR-0028 was **superseded**, not overwritten.

`adlc audit`: 0 orphan code; new SRS orphan until stories/code — expected.

## New / rebound SRS

| ID | Title | Parent REQ | Path |
|---|---|---|---|
| **[SRS-EP-52](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor)** | On-device pen-button map editor | [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) | `epaper/tool-modes/srs-ui.md` (`needs_design: yes`, **epaper-device**) |
| **[SRS-EP-53](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author)** | On-device pen-button map authoring | REQ-18 | `epaper/tool-modes/srs-logic.md` |
| Scene graph | Hub + Click list + Hold-move list | REQ-18 | `epaper/tool-modes/srs-ui-multi-scene.md` |

**Rebind (same ids):**

| ID | What changed |
|---|---|
| [SRS-EP-41](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch) | Click = `toggle_pen_freeform` · `toggle_pen_eraser` · `off` (drop `undo`). Hold-move = `temp_erase` · `drag_node_under_tip` · `off` (drop `temp_sel_freeform` / `temp_sel_rect`). 1-button Hold-move default **`temp_erase`**. Latch still at button-down |
| [SRS-EP-42](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool) | Chip mirror **only** — not the editor. States: `barrel.hold_temp_erase` · `barrel.hold_drag_under_tip` · `barrel.click_toggled` · `barrel.capability_0`. Dropped temp-freeform/rect chip states |
| [SRS-EP-43](../../../.docs/modules/epaper/features/tool-modes/srs-quality.md#srs-ep-43-barrel-quality) | Default Hold-move = temp erase; editor usable offline; closed-list counts |
| [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) | Persist/restore **from tablet**. Title: persist and restore. 0 editor screens |
| [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) | **`lifecycle: retired`**. `superseded-by: [SRS-EP-52]`. Id kept. Do not paint |
| [SRS-IN-25](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-25-map-publish-quality) | Persist-up / restore-down / pending-live wins / 0 Infini screens |

Chip [SRS-EP-42] is **not** the editor. Do **not** parent EP-056 only on EP-41/EP-42.

## ADR

| ID | Title | Status |
|---|---|---|
| [ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) | Tablet authors pen-button map; Infini persist/restore | **accepted** (supersedes ADR-0028; amends ADR-0015 family + ADR-0025 UI home) |
| [ADR-0028](../../../.docs/adr/ADR-0028-pen-button-map-settings-channel.md) | Pen-button map publish is a settings channel | **superseded** — settings family on `:9877` **stands**; do **not** implement Infini-as-author or publish-on-desktop-save |

Payload: `pen_capability` tablet→desktop; `pen_button_map` tablet→desktop on live bind; desktop→tablet **restore only** on hello when this tablet session has not authored. Hello race: pending live map **wins**. Anatomy: [domain/pen-button-map](../../../.docs/domain/pen-button-map.md) (no REQ ids). `map.stale` withdrawn.

## Story retarget table (SM — do not apply in this run)

| Story | Action | Proposed `parent_srs` |
|---|---|---|
| **[STORY-EP-056](../stories/STORY-EP-056.md)** | Set `parent_srs` to the editor. Dual-ask Designer + QA. Same package `pen-button-map/`. Platform **epaper-device** | **[SRS-EP-52]** (logic SRS-EP-53, quality SRS-EP-43, graph `srs-ui-multi-scene.md`). Optional chip HTML: SRS-EP-42 — not overlay scenes |
| [STORY-IN-034](../stories/STORY-IN-034.md) | Keep id `done` (historical Infini paint). Do not treat as shipping UI | — |
| [STORY-IN-035](../stories/STORY-IN-035.md) | Unfreeze persist/restore. Drop “assign slots and save on desktop”. `needs_design: no` on Infini | SRS-IN-23, SRS-IN-25. Parent Infini REQ-05 |
| [STORY-EP-052](../stories/STORY-EP-052.md) | Replan AC: default Hold-move **Temporary eraser**; catalogues as domain. `depends_on` EP-056 only for editor chrome, not for dispatch catalogues | SRS-EP-41, SRS-EP-43 (authoring writes SRS-EP-53) |

## Designer constraints (EP-056)

- **Platform:** epaper-device (`data-platform: epaper`). 1-bit, **no hover**, no focus, no cursor, no motion. Finger-eligible ≥ **64 du**.
- **Package:** `.plan/iter-005/design/pen-button-map/` — **Epaper**, not Infini. Historical IN-034 slate is not Keep.
- **Scene ids (bind):**
  - `scene.pen_map_editor` — hub (`map.layout_0` / `_1` / `_2` / `map.offline`)
  - `scene.pen_map_click` — Click list only: current↔Freeform Select · current↔Eraser · Off
  - `scene.pen_map_hold` — Hold-move list only: Temporary eraser · Drag node under tip · Off
- **Nav kinds:** `present-modal` (hub) · `present-sheet` (lists) · `dismiss`.
- **Entry:** `cta.pen_map_open` placement is **unnamed** — you propose; PM adopts. Must **not** be Infini File menu, a 5-way radio on exclusive-tool tiles, or a fourth `toolMode`.
- **Out of this overlay:** chip during hold-move ([SRS-EP-42](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool)) — Temporary eraser **mirrors**; Drag-under-tip does **not** switch exclusive tool.
- **Infini:** 0 map-editor screens. Persist is not a design story.

## Review (review-design)

### Strengths

- Author vs persist is MECE: Epaper writes live ([SRS-EP-53]); Infini stores ([SRS-IN-23]); dispatch stays [SRS-EP-41].
- Closed catalogues match PRD 2026-08-20; dropped ids cannot return via Designer invent.
- Settings family remains type-auditable (0 `doc_*`). Hello race is explicit (pending live wins).
- Scene graph: every Keep scene has REQ-18 + SRS-EP-53; pickers have cancel = 0 write (orphan C4).

### Concerns (accepted unless noted)

- `cta.pen_map_open` placement unnamed (GAP-01). Product already accepted; not a missing scene.
- REQ-18 dual-ask still lists **chip** hold-move journeys. Those are SRS-EP-42, not the editor graph. SM/Designer must not bury them in follow/hand-touch, and must not treat them as Infini screens.
- [`.docs/design/index.md`](../../../.docs/design/index.md) still lists Infini `[UI-IN-03]` / SRS-IN-24. Architect did not edit the design index. Designer retargets on EP-056.
- Shipped Qt/Electron still Infini-author / old catalogues. Spec lag is expected; do not AC the old direction.

### Risks

None that block EP-056. Sensitivity: **live map while Infini is down** vs **restore after a later session**. Trade-off pinned in ADR-0030 (in-session author = tablet; across-restart store = Infini).

## Constraints (lock)

Vertical, stop `verified`, bounded, wip 2, modules epaper+infini. **Forbidden:** REQ-15 tables, REQ-08, CHL-0011 / CHL-0012, EP-032, AI, last-writer ADR-0023. Do not `/dev` until design done + BDD.

## Next

`/sm` set STORY-EP-056 `parent_srs` → **SRS-EP-52**, then `/designer` EP-056. Then `/qa` BDD. Persist implement (IN-035) after the bind; do **not** schedule an Infini map settings screen.
