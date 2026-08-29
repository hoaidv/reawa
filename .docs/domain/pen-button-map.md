---
entity: PenButtonMap
slug: pen-button-map
lifecycle: active
owner: architect
owning_context: device-settings
---

# Pen-button map

## Definition

The assignment of each stylus **barrel button** to exactly one **Click** catalogue item and exactly one **Hold-move** catalogue item. The **tablet authors** the live map and **persists it on this Epaper device**. Infini is **not** persist home. It is **not** a document field ([ADR-0031](../adr/ADR-0031-device-settings-persist-on-epaper.md), supersedes [ADR-0030](../adr/ADR-0030-tablet-authors-pen-button-map.md)).

## Anatomy

| Field / part | Type / shape | Notes |
|---|---|---|
| `mapId` | uuid | Idempotency / apply identity |
| `buttons[].index` | 1 \| 2 | Hardware index; absent indexes are not stored |
| `buttons[].click` | click-catalogue id | Exactly one of the closed Click set |
| `buttons[].holdMove` | hold-move-catalogue id | Exactly one of the closed Hold-move set |
| `capability.buttonCount` | 0 \| 1 \| 2 | Reported by the tablet HID; UI must not invent slots |

### Click catalogue (closed)

| id | Meaning |
|---|---|
| `toggle_pen_freeform` | Toggle exclusive tool `pen` ↔ `sel_freeform` |
| `toggle_pen_eraser` | Toggle `pen` ↔ **last-used eraser** (`erase_brush` \| `erase_area` \| `erase_object` — not the nib) |
| `off` | No-op |

**Not in v1:** `undo` (Undo stays on the ToolChip). Unknown ids apply as `off`.

### Hold-move catalogue (closed)

| id | Meaning |
|---|---|
| `temp_erase` | Temporary **last-used eraser** from `pen` until release, then snap back (**not** the nib channel). Already-in-eraser → no-op |
| `drag_node_under_tip` | Move hittable node under tip; empty canvas → no-op, **0** lasso |
| `off` | No-op |

**Not in v1:** `temp_sel_freeform`, `temp_sel_rect` (Hold-move snaps back; a temporary select that does nothing afterward is meaningless). A combined “empty→lasso / node→drag” item may appear later only as its **own** named id.

### Defaults

| Capability | Button 1 | Button 2 |
|---|---|---|
| 1-button | Click `toggle_pen_freeform`, Hold-move `temp_erase` | — |
| 2-button | Same as 1-button | Click `toggle_pen_eraser`, Hold-move `temp_erase` |
| 0-button | No slots | — |

## Invariants

- Each present slot binds **exactly one** catalogue item per side (Click, Hold-move). Never three jobs on one hold-while-moving gesture.
- Unknown ids apply as `off` on the device (no crash, no invented binding).
- Rebind does not mutate a gesture already latched at button-down.
- Eraser **nib** is not a catalogue item ([ADR-0025](../adr/ADR-0025-barrel-vs-eraser-nib.md)).
- Live map applies even when Infini is down. Persist is **on this device** and does **not** wait on Infini. 0 lost local binds.
- Infini never authors catalogue ids. Infini never stores or restores the map.
- A different Epaper device does not inherit this map from an Infini document.

## Relationships

| Related | Cardinality | Notes |
|---|---|---|
| This Epaper device | 1:1 durable map | Device-local store; survives app restart on **this** device |
| Session | none for persist | Optional `pen_capability` HID telemetry T→D. **0** `pen_button_map` messages |
| Vector document | none | Must not appear in SVG / `doc_change` |

## States

| State | Meaning | Transitions |
|---|---|---|
| `map.absent` | 0-button or never bound this device (factory default until first bind or device-store load) | → `map.applied` on editor bind or device-store restore at start |
| `map.applied` | Device using this `mapId`; flushed to the device store | → new `mapId` on next editor bind (next gesture) |

`map.pending_persist` (“wait for Infini to store”) is **withdrawn** — Infini does not persist. `map.stale` (“Infini edited offline”) remains withdrawn — Infini does not author.

## Non-goals / out of scope

- Visual layout of the on-device Settings page (Designer / [SRS-EP-52](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor))
- Infini desktop settings chrome ([SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) retired)
- Infini persist/restore ([SRS-IN-23](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) retired)
- Digitizer eraser-nib routing ([ADR-0025](../adr/ADR-0025-barrel-vs-eraser-nib.md))
- ToolChip as a 5-way radio (chip may **mirror** temporary erase during hold-move; it is not the Settings page)
- Other Settings master items this package (only **Pen buttons**)

## Linked modules / features

| Module | Feature / SRS | Role |
|---|---|---|
| `epaper` | [SRS-EP-52](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) / [SRS-EP-53](../modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author) | Settings shell + author live map + persist on device |
| `epaper` | [SRS-EP-41](../modules/epaper/features/tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch) | consume + dispatch |
| `epaper` | [SRS-EP-42](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool) | chip mirror during hold-move Temporary eraser — **not** the Settings page |
| `infini` | [SRS-IN-23](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) | **retired** — 0 persist, 0 restore |
