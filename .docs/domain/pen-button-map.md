---
entity: PenButtonMap
slug: pen-button-map
lifecycle: active
owner: architect
owning_context: session-settings
---

# Pen-button map

## Definition

The persisted assignment of each stylus **barrel button** to exactly one **Click** catalogue item and exactly one **Hold-move** catalogue item. Infini authors and stores it; Epaper consumes it. It is **not** a document field ([ADR-0028](../adr/ADR-0028-pen-button-map-settings-channel.md)).

## Anatomy

| Field / part | Type / shape | Notes |
|---|---|---|
| `mapId` | uuid | Idempotency / apply identity |
| `buttons[].index` | 1 \| 2 | Hardware index; absent indexes are not stored |
| `buttons[].click` | click-catalogue id | Exactly one of the closed Click set |
| `buttons[].holdMove` | hold-move-catalogue id | Exactly one of the closed Hold-move set |
| `capability.buttonCount` | 0 \| 1 \| 2 | Reported by the tablet (`pen_capability`); UI must not invent slots |

### Click catalogue (closed)

| id | Meaning |
|---|---|
| `toggle_pen_freeform` | Toggle exclusive tool `pen` ↔ `sel_freeform` |
| `toggle_pen_eraser` | Toggle `pen` ↔ eraser command (Path B / temporary eraser arm — not the nib) |
| `undo` | One undo |
| `off` | No-op |

### Hold-move catalogue (closed)

| id | Meaning |
|---|---|
| `temp_sel_freeform` | Temporary `sel_freeform` until release |
| `temp_sel_rect` | Temporary `sel_rect` until release |
| `temp_erase` | Temporary stroke-erase **feel** (Path A mutation; **not** the nib channel) |
| `drag_node_under_tip` | Move hittable node under tip; empty canvas → no-op, **0** lasso |
| `off` | No-op |

**Not in v1:** a combined “empty→lasso / node→drag” item. That may appear later only as its **own** named id.

### Defaults

| Capability | Button 1 | Button 2 |
|---|---|---|
| 1-button | Click `toggle_pen_freeform`, Hold-move `temp_sel_freeform` | — |
| 2-button | Same as 1-button | Click `toggle_pen_eraser`, Hold-move `temp_erase` |
| 0-button | No slots | — |

## Invariants

- Each present slot binds **exactly one** catalogue item per side (Click, Hold-move). Never three jobs on one hold-while-moving gesture.
- Unknown ids apply as `off` on the device (no crash, no invented binding).
- Rebind does not mutate a gesture already latched at button-down.
- Eraser **nib** is not a catalogue item ([ADR-0025](../adr/ADR-0025-barrel-vs-eraser-nib.md)).

## Relationships

| Related | Cardinality | Notes |
|---|---|---|
| Session | 1:1 live map | Published on `:9877` as `pen_button_map` |
| Vector document | none | Must not appear in SVG / `doc_change` |

## States

| State | Meaning | Transitions |
|---|---|---|
| `map.absent` | 0-button or never published | → `map.applied` on first valid publish |
| `map.applied` | Device using this `mapId` | → new `mapId` on next publish (next gesture) |
| `map.stale` | UI edited offline, not yet published | → `map.applied` after reconnect publish |

## Non-goals / out of scope

- Visual layout of the Infini editor (Designer / [SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md))
- Digitizer eraser-nib routing ([ADR-0025](../adr/ADR-0025-barrel-vs-eraser-nib.md))
- ToolChip as a 5-way radio (PRD: tablet does not host that)

## Linked modules / features

| Module | Feature / SRS | Role |
|---|---|---|
| `infini` | [SRS-IN-23](../modules/infini/features/tablet-sync/srs-logic.md) / [SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md) | persist + publish + settings UI |
| `epaper` | [SRS-EP-41](../modules/epaper/features/tool-modes/srs-logic.md) | consume + dispatch |
