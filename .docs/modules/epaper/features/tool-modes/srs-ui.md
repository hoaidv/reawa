---
feature: tool-modes
parent_req: [REQ-03]
version: 0.3.0
lifecycle: active
needs_design: true
---

# SRS — Tool modes Epaper (UI)

Durable UI contract for the on-device toolbar. This is the **first chrome ever placed on the
Epaper panel** — everything before it was full-bleed ink ([region-sync srs-ui](../region-sync/srs-ui.md)).
Logic: [SRS-EP-04](./srs-logic.md). Quality: [SRS-EP-06](./srs-quality.md).
Decision: [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1, as amended by
[ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md).
Selection and manipulation chrome: [SRS-EP-12](../ink-box/srs-ui.md).

## [SRS-EP-05] On-device tool chip {#srs-ep-05-tool-chip}

<!-- adopted CHL-0003 2026-08-11: floating orientation-top chip; CHL-0019 2026-08-15: 64×64 tiles -->
<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Selection affordances are real, not ghosts; adds
     the session/publish status affordance; tools no longer go unavailable when the link drops.
     Same id, content revised. -->

> **Revised 2026-08-14 (ADR-0021).** Exclusive tools are three; enclose and connectors are
> recognizer toggles on `Pen`, not tools. Selection still dims both toggles. Publish status
> stays on the chip. Detailed selection chrome remains [SRS-EP-12](../ink-box/srs-ui.md).

### Design authority

1. This `srs-ui.md`
2. `epaper` REQ-03 acceptance
3. Physical constraints below (they outrank aesthetics)
4. Design package `[UI-EP-01]` (`.plan/iter-003/design/epaper-tool-strip/`) — scenes + Spec
5. `.docs/DESIGN.md` tokens — **advisory only**; the desktop design system does not transfer to
   a 1-bit panel

### Purpose

**One job:** let the creator see and change what the pen will do, without ever costing ink
latency or reserving a full edge band of drawing area.

### Physical constraints (binding)

| Constraint | Value | Consequence for design |
|---|---|---|
| Panel | 1404 × 1872, **1-bit** e-ink | No color, no greyscale hierarchy, no shadows/blur |
| Full refresh floor | ~250 ms, ghosting allowed | Never depend on a settled frame to convey state |
| Partial refresh | Chip bounds only | Chrome must be a small, isolatable rect (not a full edge band) |
| Input | Pen (Wacom EMR) + capacitive touch — **touch unverified from Qt** | Fallback path must exist (`pen-on-chip`) |
| Chip size | Height **64 px**; tools **64×64** icon tiles; hug width | Compact chip — **relaxes** the prior ≥120 px finger-target rule (CHL-0003); **64** adopted [CHL-0019](../../../../../.plan/iter-004/challenges/CHL-0019-toolchip-tile-size.md) after RM2 verify that 32 px was too small |
| Ambient | Reflective display, read in any light | Contrast by shape and fill, never by tint |

### Composition layers (binding)

| Layer id | Role | Fill |
|---|---|---|
| InkSurface | **Full-bleed** drawing area (existing) | White |
| ToolChip | Floating strip: **3 exclusive tools** + gap + **2 recognizer toggles** + gap + Undo/Redo ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md), [ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md)) | White clusters; 1 px outline; squared (`border-radius: 0`) |
| SelectionOverlay | Handles + ghost while `selection` is active | Outline only |
| StatusLine | Existing debug/status text | Unchanged |

**Containment:** `ToolChip` is a **floating** compact control cluster (height **64 px**, width hug
content, icon-only) anchored to the **top edge of the current gut orientation** (moves with
device orientation; when gut is on top, oriented “top” places the chip near the opposite short
edge). `InkSurface` remains **full-bleed** — chrome does **not** reserve a full band. Pen/touch
hits on the chip are excluded from ink via hit-test ([SRS-EP-04](./srs-logic.md)); exclusion rect
= **chip bounds**, not a full edge strip.

### Layout regions

| # | Region id | Parent | Contents |
|---|---|---|---|
| 0 | DeviceScreen | panel | Full panel |
| 1 | ToolChip | DeviceScreen | 3 exclusive tools, gap, 2 recognizer toggles, gap, Undo + Redo; floating orientation-top |
| 2 | InkSurface | DeviceScreen | Full-bleed drawing region |
| 3 | SelectionOverlay | InkSurface | Bounds + handles for the selected node |
| 4 | StatusLine | DeviceScreen | Existing status text |

### Closed control inventory

| id | Control | Region |
|---|---|---|
| `tool.sel_rect` | Selection rect (AABB marquee) | ToolChip |
| `tool.sel_freeform` | Selection freeform (lasso) | ToolChip |
| `tool.pen` | Pen tool (**default**) | ToolChip |
| `tgl.recog.ink_box` | Ink-box recognition (independent; ships armed) | ToolChip |
| `tgl.recog.connector` | Connector recognition (independent; ships armed) | ToolChip |
| `ind.tool_active` | Which exclusive tool is armed | ToolChip |
| `ind.recog_armed` | Toggle on / off / dimmed-under-Selection | ToolChip |
| `ind.tool_unavailable` | Tool cannot act (below LOD cutoff; touch layer dead) | ToolChip |
| `ind.publish_status` | Linked · changes queued · reloading document | ToolChip |
| `cta.undo` | Undo last structural gesture | ToolChip history cluster ([ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md)) |
| `cta.redo` | Redo last undone gesture | ToolChip history cluster |
| `ovl.selection_bounds` | Selected node bounds | SelectionOverlay ([SRS-EP-12](../ink-box/srs-ui.md)) |
| `ovl.resize_handles` | Resize handles on bounds | SelectionOverlay ([SRS-EP-12](../ink-box/srs-ui.md)) |

`tool.ink_box` is **removed** ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md) supersedes
[ADR-0017](../../../../adr/ADR-0017-four-tool-chip.md)). `ovl.drag_ghost` is **removed** — the ink
itself moves. No brushes, colors, layers, or document browser ([epaper Non-Goals](../../prd.md)).
Undo/Redo are **actions**, not exclusive tools. Recognizer toggles are **not** `toolMode`.
Viewport-follow is **[SRS-EP-50](../region-sync/srs-ui.md#srs-ep-50-follow-toggle)** — **not** a fourth exclusive tool, recognizer, or hand-tool tile. Do not add `btn.viewport_follow` to this inventory.

### Interaction map

| Control | Action | Result | Feedback |
|---|---|---|---|
| `tool.*` | Finger tap (or pen-on-chip fallback) | Arm that exclusive tool | Active indicator moves within **300 ms** (partial refresh of chip) |
| `tgl.recog.ink_box` / `tgl.recog.connector` | Finger tap while `pen` is active | Flip armed / disarmed | Toggle invert within **300 ms**; does **not** change the exclusive tool |
| `tgl.recog.*` | Tap while a Selection tool is active | No-op | Toggles stay **dimmed**; armed state retained |
| `cta.undo` / `cta.redo` | Finger tap (or pen-on-chip) | Restore previous / undone tree ([SRS-EP-07](../device-document/srs-logic.md)) | Brief invert; **does not** change `toolMode`. Empty stack = no-op |
| `ovl.selection_bounds` | Pen press inside | Select + begin move | Bounds outline appears; the **ink** follows the pen |
| `ovl.resize_handles` | Pen drag on handle | Resize | Real ink resizes live; committed geometry = released geometry |
| InkSurface empty | Pen press in `selection` | Clear selection | Overlay disappears, leaving 0 residual pixels |
| `ind.publish_status` | — (indicator only) | — | Reflects link + queue state; never blocks a tool |

### Control states

Note there is **no hover and no focus** on this platform — do not design them.

| Control | default | active (armed) | pressed | unavailable |
|---|---|---|---|---|
| `tool.sel_rect` | outline | filled / inverted | brief invert | hatched + inert **only** below the LOD cutoff |
| `tool.sel_freeform` | outline | filled / inverted | brief invert | hatched + inert **only** below the LOD cutoff |
| `tool.pen` | outline | filled / inverted | brief invert | never — always available |
| `tgl.recog.ink_box` | outline (disarmed) | filled when armed | brief invert | **dimmed** (not tappable) while a Selection tool is active; armed state kept |
| `tgl.recog.connector` | outline (disarmed) | filled when armed | brief invert | **dimmed** (not tappable) while a Selection tool is active; armed state kept |
| `cta.undo` / `cta.redo` | outline | never armed | brief invert | empty stack still tappable (no-op) |

**No tool is gated by the session.** The link state is reported by `ind.publish_status`, not by
disabling the creator's tools.

**Active and armed states must be readable from shape/fill alone**, because a trailing refresh can
leave a ghost of the previous state on screen. Dimmed toggles must stay distinguishable from
disarmed.

### States matrix

| State id | ToolChip | InkSurface | SelectionOverlay |
|---|---|---|---|
| `tool.pen` | Pen armed; both toggles visible | Ink under pen | hidden |
| `recog.ink_box.on` / `.off` | Ink-box toggle armed / disarmed | unchanged | hidden |
| `recog.connector.on` / `.off` | Connector toggle armed / disarmed | unchanged | hidden |
| `recog.rejected` | Pen still armed; toggle still armed | Stroke stayed ordinary ink | hidden; **no** banner |
| `tool.sel_rect.idle` | Rect armed; both toggles **dimmed** | No ink from pen | hidden |
| `tool.sel_freeform.idle` | Freeform armed; both toggles **dimmed** | No ink from pen | hidden |
| `tool.selection.selected` | Either selection tool armed; toggles dimmed | — | bounds + handles |
| `tool.selection.moving` | Either selection tool armed; toggles dimmed | **Real ink moving under the pen** | bounds tracking the ink |
| `tool.selection.resizing` | Either selection tool armed; toggles dimmed | **Real ink resizing** per `inkScaleMode` | bounds + active handle |
| `session.linked` | `ind.publish_status` = linked | unchanged | unchanged |
| `session.pending_changes` | `ind.publish_status` = queued (**tools still usable**) | Pen still inks; all editing works | unchanged |
| `session.reloading` | `ind.publish_status` = reloading | Document being replaced | hidden |
| `manipulation.unavailable` | Selection hatched + inert | No grab | hidden; chip states why |
| `touch.unavailable` | Chip inert for finger; pen-on-chip or pen forced | Pen inks | hidden; status line explains |
| `orient.gutOnTop` | Chip on oriented top (near opposite short edge) | Full-bleed | unchanged |

`tool.ink_box`, `tool.selection.dragging` (ghost), and `session.down` (tools disabled) are
**retired states** — do not design them.

### Platform profile

| Field | Value |
|---|---|
| Profile | **epaper-device** (reMarkable 2, Qt/QML fullscreen) |
| `data-platform` | `epaper` |
| Preview | Landscape tablet frame **1872×1404** (native panel rotated) — not phone chrome |
| Input | Pen for content; finger for chrome (pen-on-chip fallback). No hover, no keyboard, no cursor |
| Color | 1-bit — design in pure black/white with fill and hatch only |
| Motion | **None.** No transitions, no fades — every animation is a refresh cost |

### Anti-patterns

- A **full-band** edge strip that shrinks `InkSurface` (retired by CHL-0003).
- Chrome that uses rounded “pill” chrome, or a **full-band** strip. Tile size is **64×64** (CHL-0019), not 32.
- Conveying active state by tint, greyscale, shadow, or animation.
- A tool that can leave the creator unable to draw (pen must always be reachable).
- Reusing desktop `tokens.css` sizing as if the chip were a desktop toolbar.
- Refreshing the full panel to show a tool change.
- Phone-sized mobile chrome in design previews for this surface.

### Out of scope (UI)

- Pan / zoom **chrome** (two-finger has no dedicated tool tile — [SRS-EP-22](../ink-box/srs-ui.md#srs-ep-22-hand-touch-ui)).
- Viewport-follow icon toggle ([SRS-EP-50](../region-sync/srs-ui.md#srs-ep-50-follow-toggle)) — not this chip.
- Device Settings / leading 10 mm stylus-with-barrels tile (`cta.pen_map_open`) — **sibling** of ToolChip ([SRS-EP-52](#srs-ep-52-pen-map-editor), GAP-01), **not** this chip’s inventory, **not** a fourth exclusive tool.
- Page navigation, document browser.
- Rotation, multi-select, marquee, align/distribute affordances — [REQ-08](../../prd.md#node-manipulation).
- A `doc_load` confirmation dialog — the handshake makes a load safe by construction; the creator
  sees `session.reloading`, not a decision.

### Open (needs design)

- **Undo affordance.** **Closed** [CHL-0016](../../../../../.plan/iter-003/challenges/CHL-0016-undo-redo-toolbar.md)
  / [ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md): `cta.undo` and `cta.redo` after a
  gap on the primary strip. Exclusive tools are three ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md)).
- **Recognizer toggle glyphs** — armed / disarmed / dimmed-under-Selection must be distinct on
  1-bit partial refresh. Binding inventory is closed here; painting is [STORY-EP-026](../../../../../.plan/iter-004/stories/STORY-EP-026.md).
- **`inkScaleMode` toggle placement** — it belongs to a selected box, not to the chip; specified in
  [SRS-EP-12](../ink-box/srs-ui.md).

---

## [SRS-EP-29] Erase chrome {#srs-ep-29-erase-ui}

<!-- lifecycle: retired -->
<!-- superseded-by: [SRS-EP-54] -->
<!-- note: 2026-08-29 CHL-0028. Chrome is prd-erase.md + SRS-EP-54…56. -->

**Retired.** Do not implement Path A/B chrome. Canonical: [prd-erase.md](../../prd-erase.md) · [SRS-EP-54](../erase/srs-logic.md#srs-ep-54-erase-mode).

---

## [SRS-EP-42] Chip mirrors temporary tool {#srs-ep-42-chip-temp-tool}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** [REQ-18](../../prd.md#pen-buttons). **Logic:** [SRS-EP-41](./srs-logic.md#srs-ep-41-barrel-dispatch). **Not the map editor** — that is [SRS-EP-52](#srs-ep-52-pen-map-editor).

### Purpose

When Hold-move is **Temporary eraser**, show that temporary tool **during hold-move**, then restore unless Click toggled. Drag-under-tip does **not** switch the exclusive tool. Tablet does **not** host a 5-way radio on chip tiles.

### Closed inventory (chip only)

Reuse `ind.tool_active`. **0** barrel-map editor controls on the chip.

### States (do not add dropped catalogues)

`barrel.hold_temp_erase` · `barrel.hold_drag_under_tip` · `barrel.click_toggled` · `barrel.capability_0` (slots absent)

**Dropped:** `barrel.hold_temp_freeform` · `barrel.hold_temp_rect`.

### Hit

Chip tiles remain 64 du. Barrel itself has no on-panel hit target.

---

## [SRS-EP-52] Device Settings page (Pen buttons) {#srs-ep-52-pen-map-editor}

<!-- lifecycle: active -->
<!-- needs_design: yes -->
<!-- revised: 2026-08-20 — CHL-0025 Settings shell + inline catalogues; GAP-01 entry tile; persist is REQ-20. Same id. -->

**Parent:** [REQ-20](../../prd.md#device-settings) (shell, entry, persist home). Catalogues: [REQ-18](../../prd.md#pen-buttons). **Logic:** [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author). **Quality:** [SRS-EP-43](./srs-quality.md#srs-ep-43-barrel-quality). **Scene graph:** [srs-ui-multi-scene.md](./srs-ui-multi-scene.md). **Decision:** [ADR-0031](../../../../adr/ADR-0031-device-settings-persist-on-epaper.md). **Platform:** **epaper-device** (`data-platform: epaper`). **Do not parent on [SRS-EP-05](#srs-ep-05-tool-chip)** (ToolChip) or [SRS-EP-42](#srs-ep-42-chip-temp-tool) (chip mirror). Infini [SRS-IN-24](../../../infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) is **retired** — do not paint desktop map chrome. Catalogues: [domain/pen-button-map](../../../../domain/pen-button-map.md). Do **not** invent other Settings master items.

### Design authority

1. This section + [srs-ui-multi-scene.md](./srs-ui-multi-scene.md)
2. [REQ-20](../../prd.md#device-settings) + [REQ-18](../../prd.md#pen-buttons) acceptance
3. Physical constraints (1-bit, no hover, partial refresh) — they outrank aesthetics
4. Design package `.plan/iter-005/design/pen-button-map/` — scenes + Spec ([UI-EP-08](../../../../../.plan/iter-005/design/pen-button-map/ui-spec.md))
5. `.docs/DESIGN.md` tokens — **advisory only**; desktop system does not transfer

### Purpose

**One job:** let the creator open **one Settings page**, pick **Pen buttons** in the master list, and bind each **present** barrel slot (Click and Hold-move) **inline** in the detail pane. Not document chrome. Not a ToolChip exclusive tool. Not a sheet.

### Physical constraints (binding)

Same panel profile as [SRS-EP-05](#srs-ep-05-tool-chip): 1404 × 1872, **1-bit**, no hover, no focus, no cursor, no motion. Finger-eligible hits on the Settings page ≥ **64 du** (CHL-0019 floor). **Exception (GAP-01 adopted):** `cta.pen_map_open` is a lone **10 mm** tile. Full-panel Settings is an isolatable rect (partial refresh of the page preferred).

### Composition / containment (contract, not craft)

| Region | Parent | Role |
|---|---|---|
| DeviceScreen | panel | Full panel / drawing underlay |
| PenMapOpen | DeviceScreen | Leading 10 mm entry tile — **sibling of ToolChip**, not inside exclusive-tool cluster, not follow toggle |
| SettingsShell | DeviceScreen | Full-panel **master-detail** Settings page |
| MasterList | SettingsShell | First (only this package) item **Pen buttons** |
| DetailPane | SettingsShell | Pen buttons detail — slot rows + **inline** catalogues |
| SlotRow | DetailPane | One row per present button (index 1 or 2) |
| SlotClick | SlotRow | Closed Click radios **in this pane** (not a sheet) |
| SlotHoldMove | SlotRow | Closed Hold-move radios **in this pane** (not a sheet) |

`ClickList` / `HoldList` as `present-sheet` scenes are **retired** ([CHL-0025](../../../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md)). Catalogues live in the detail pane.

**Entry (GAP-01 adopted).** `cta.pen_map_open`: lone **10 mm** 1-bit tile, stylus-with-barrels glyph, floating orientation-top **leading**, sibling of ToolChip (same chrome family as viewport-follow trailing and Undo). **Not** Infini File menu, **not** a 5-way radio on `tool.*` tiles, **not** a fourth exclusive tool.

### Closed control inventory

| id | Kind | Notes |
|---|---|---|
| `cta.pen_map_open` | entry | Leading 10 mm tile (GAP-01) |
| `cta.pen_map_close` | dismiss | Return to drawing; live map kept |
| `nav.settings.pen_buttons` | master item | Only master row this package |
| `slot.click` | value + inline pick | Per present button — **not** a sheet hop |
| `slot.hold_move` | value + inline pick | Per present button — **not** a sheet hop |
| `list.click.toggle_pen_freeform` | pick | Current primary ↔ Freeform Select |
| `list.click.toggle_pen_eraser` | pick | Current primary ↔ Eraser |
| `list.click.off` | pick | No-op |
| `list.hold.temp_erase` | pick | Temporary eraser |
| `list.hold.drag_node_under_tip` | pick | Drag node under tip |
| `list.hold.off` | pick | No-op |

Designer **must not** add `undo`, `temp_sel_freeform`, `temp_sel_rect`, an eraser-nib slot, or a second master item.

### States matrix (journeys from PRD — do not add)

| State id | Scene | When |
|---|---|---|
| `map.entry` | drawing | Leading tile rest / pressed / open |
| `map.layout_0` | `scene.pen_map_editor` | 0-button: **0** slot rows (0 fake bindings) |
| `map.layout_1` | `scene.pen_map_editor` | 1-button: one row |
| `map.layout_2` | `scene.pen_map_editor` | 2-button: two rows |
| `map.offline` | `scene.pen_map_editor` | Session down; page still usable; persist is on-device (does not wait) |
| `map.rebound_next_gesture` | underlay | Bind committed; in-flight barrel gesture unchanged |

`map.slot_click` / `map.slot_hold` as **scene** states are **retired** — catalogues are intra-scene. Chip hold-move states stay on [SRS-EP-42](#srs-ep-42-chip-temp-tool) — **out of this page**.

### Interaction map

| Control | Action | Result |
|---|---|---|
| `cta.pen_map_open` | tap | `present-modal` → `scene.pen_map_editor` (Pen buttons selected); exclusive tool unchanged |
| `cta.pen_map_close` | tap | `dismiss`; live map kept |
| `list.click.*` / `list.hold.*` | tap | Write that id **in place**; **0** sheet hops; persist on device ([SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author)) |

There is **no** list-cancel hop (write is in-place). Dismiss Settings does not revert an in-place pick.

### UI-driving fields

`pen.buttonCount`, `pen.map.buttons[].click`, `pen.map.buttons[].holdMove`, `session.connected` — Designer must not invent a third slot type, a second master item, or disable the page when the session is down. `session.connected` may label offline copy; it **does not** gate editing or persist.

### Anti-patterns

- Infini / Electron / slate desktop settings chrome
- Hover, focus, cursor, color, motion
- `present-sheet` Click / Hold-move lists
- Undo on the Click catalogue
- Temporary freeform / rect on the Hold-move catalogue
- 5-way radio on exclusive-tool tiles
- Treating `cta.pen_map_open` as a fourth exclusive tool
- Saving the map into the SVG or Infini app settings
- Gating the page on `session.connected`
- Inventing other Settings master items

### Dual-ask

`/designer` Spec + scenes already in [UI-EP-08](../../../../../.plan/iter-005/design/pen-button-map/ui-spec.md) (`map.entry` · `map.layout_0` · `map.layout_1` · `map.layout_2` · `map.offline`). Chip mirror scenes belong to [SRS-EP-42](#srs-ep-42-chip-temp-tool). `/qa` BDD from [REQ-20](../../prd.md#device-settings) AC + [REQ-18](../../prd.md#pen-buttons) catalogue AC + [SRS-EP-43](./srs-quality.md#srs-ep-43-barrel-quality).

---

## [SRS-EP-47] Manual create chrome {#srs-ep-47-manual-create-ui}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** [REQ-17](../../prd.md#manual-create) (Should). **Logic:** [SRS-EP-44](./srs-logic.md#srs-ep-44-manual-create-routing). **Platform:** epaper-device.

### Closed create ids

`create.frame` · `create.connector` · `create.attach` · `create.primitive.ellipse` · `create.primitive.rect` · `create.primitive.line`

Finger-eligible controls ≥64 du. Primitive place is pen-precise (drawn bounds).

### States (from PRD)

`create.entry` · `create.frame_place` · `create.connector_place` · `create.primitive_place` · `create.cancel` · `create.vs_pen_ink`

### Anti-patterns

- Brush / color / layer palette
- Re-specifying ink-box enclose as a new create tool
