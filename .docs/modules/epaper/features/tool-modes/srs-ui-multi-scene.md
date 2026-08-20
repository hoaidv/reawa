---
feature: tool-modes
parent_req: [REQ-18]
version: 0.1.0
lifecycle: active
purpose: Multi-scene UX for the on-device pen-button map editor — hub + Click list + Hold-move list
---

# SRS — Pen-button map editor (Scene graph)

Co-authors: `/pm` (REQ-18 journeys) · `/architect` (routes below) · `/designer` (composition per scene). Chip hold-move mirror is **[SRS-EP-42](./srs-ui.md#srs-ep-42-chip-temp-tool)** — not in this graph.

Platform: **epaper-device** (`data-platform: epaper`). No hover, no keyboard, no deep links.

## Navigation vocabulary (binding)

| Kind | Meaning | Typical chrome |
|---|---|---|
| `present-modal` | Overlay over the drawing surface | Parent dimmed or covered; ToolChip policy is Designer craft |
| `present-sheet` | List overlay over the editor hub | Hub remains under |
| `dismiss` | Close overlay/sheet | Parent resumes |
| `system` | N/A this graph | No OS picker |

No `push` / `tab-switch` / `deep-link` on this surface.

## Scene catalog

### Scene: `scene.pen_map_editor` — Map editor hub

| Field | Value |
|---|---|
| scene_id | `scene.pen_map_editor` |
| Purpose (one job) | Show present barrel slots (0/1/2) and current Click / Hold-move bindings |
| Presentation | `present-modal` |
| Platform container | Epaper overlay on DeviceScreen (1-bit) |
| Primary `srs-ui` | [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor) |
| Design package slug | `pen-button-map` |
| Entry points | Drawing chrome `cta.pen_map_open` (placement unnamed — Designer proposes) |
| Exit points | `cta.pen_map_close` / overlay dismiss → drawing |
| States covered | `map.layout_0` · `map.layout_1` · `map.layout_2` · `map.offline` · `map.rebound_next_gesture` |
| Logic binding | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) — write live map; persist-up if linked |
| Parent REQ | [REQ-18](../../prd.md#pen-buttons) |
| Out of scope | Infini desktop settings; 5-way radio on exclusive-tool tiles; chip temp-tool |

### Scene: `scene.pen_map_click` — Click catalogue list

| Field | Value |
|---|---|
| scene_id | `scene.pen_map_click` |
| Purpose (one job) | Pick exactly one closed Click id for the open slot |
| Presentation | `present-sheet` |
| Platform container | Sheet over hub |
| Primary `srs-ui` | [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor) |
| Design package slug | `pen-button-map` |
| Entry points | Hub `slot.click` |
| Exit points | Pick `list.click.*` → `dismiss` hub (write) · cancel → `dismiss` (0 write) |
| States covered | `map.slot_click` |
| Logic binding | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) closed Click set |
| Parent REQ | [REQ-18](../../prd.md#pen-buttons) |
| Out of scope | `undo`; extra Click ids |

### Scene: `scene.pen_map_hold` — Hold-move catalogue list

| Field | Value |
|---|---|
| scene_id | `scene.pen_map_hold` |
| Purpose (one job) | Pick exactly one closed Hold-move id for the open slot |
| Presentation | `present-sheet` |
| Platform container | Sheet over hub |
| Primary `srs-ui` | [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor) |
| Design package slug | `pen-button-map` |
| Entry points | Hub `slot.hold_move` |
| Exit points | Pick `list.hold.*` → `dismiss` hub (write) · cancel → `dismiss` (0 write) |
| States covered | `map.slot_hold` |
| Logic binding | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) closed Hold-move set |
| Parent REQ | [REQ-18](../../prd.md#pen-buttons) |
| Out of scope | `temp_sel_freeform` · `temp_sel_rect` |

## Inter-scene navigation map

| From scene | Control / trigger | Nav kind | To scene | Presentation | Chrome side-effect | Logic / REQ |
|---|---|---|---|---|---|---|
| (drawing) | `cta.pen_map_open` | `present-modal` | `scene.pen_map_editor` | overlay | Drawing under; editor usable if session down | [SRS-EP-53] · [REQ-18] |
| `scene.pen_map_editor` | `cta.pen_map_close` / dismiss | `dismiss` | (drawing) | dismiss | Overlay gone; live map kept | no write on close |
| `scene.pen_map_editor` | `slot.click` | `present-sheet` | `scene.pen_map_click` | sheet | Hub under | closed Click list |
| `scene.pen_map_editor` | `slot.hold_move` | `present-sheet` | `scene.pen_map_hold` | sheet | Hub under | closed Hold-move list |
| `scene.pen_map_click` | `list.click.*` | `dismiss` | `scene.pen_map_editor` | dismiss | Hub shows new value | write live map; persist-up if linked |
| `scene.pen_map_click` | cancel | `dismiss` | `scene.pen_map_editor` | dismiss | Unchanged | 0 write |
| `scene.pen_map_hold` | `list.hold.*` | `dismiss` | `scene.pen_map_editor` | dismiss | Hub shows new value | write live map; persist-up if linked |
| `scene.pen_map_hold` | cancel | `dismiss` | `scene.pen_map_editor` | dismiss | Unchanged | 0 write |

0-button hub: `slot.click` / `slot.hold_move` **absent** — no picker rows (A3 still holds via `cta.pen_map_close`).

## Intra-scene vs inter-scene

| Scope | Where it lives |
|---|---|
| Slot value display, 0/1/2 layout, offline copy | [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor) |
| Open/close overlay and lists | **This** navigation map |
| Catalogue ids + persist/restore | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) |

## Reference / JPEG → scene

No JPEG pack. Historical Infini slate in `pen-button-map/` is **not** Keep — Designer revises as epaper-device.

## States not in any JPEG (still required)

| Needed state / scene | Why | id |
|---|---|---|
| 0-button empty hub | PRD: 0 fake slots | `map.layout_0` |
| Session down | Live map + editor must work | `map.offline` |
| List cancel | Write-without-outcomes path | picker cancel rows above |

Chip hold-move states are **not** JPEG gaps here — they belong to SRS-EP-42.

## Traceability matrix

| scene_id | `[REQ-NN]` | `srs-logic` | UI-driving fields | `srs-ui` / design slug |
|---|---|---|---|---|
| `scene.pen_map_editor` | [REQ-18](../../prd.md#pen-buttons) | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) | `pen.buttonCount`, `pen.map`, `session.connected` | SRS-EP-52 · `pen-button-map` |
| `scene.pen_map_click` | REQ-18 | SRS-EP-53 Click catalogue | `buttons[].click` | SRS-EP-52 · `pen-button-map` |
| `scene.pen_map_hold` | REQ-18 | SRS-EP-53 Hold-move catalogue | `buttons[].holdMove` | SRS-EP-52 · `pen-button-map` |

## Orphan / gap audit

### A. Reachability

| # | Status | Notes |
|---|---|---|
| A1 | ✔ | Hub from `cta.pen_map_open`; lists from slots |
| A2 | ✔ | Drawing chrome is in-scope start. Placement unnamed, not missing |
| A3 | ✔ | Close / pick / cancel on every overlay |
| A4 | ✔ | All `dismiss` targets are hub or drawing |
| A5 | ✔ | N/A — no deep links on epaper-device |

### B. Presentation / interaction

| # | Status | Notes |
|---|---|---|
| B1 | ✔ | Kinds explicit |
| B2 | ✔ | Overlay / sheet on epaper-device |
| B3 | ✔ | Drawing under; ToolChip policy is Designer craft (not a fourth exclusive) |
| B4 | ✔ | Inventory in SRS-EP-52 |
| B5 | ✔ | Lists presented from hub slots |
| B6 | ✔ | Pickers owned by `slot.click` / `slot.hold_move` |

### C. Logic / REQ

| # | Status | Notes |
|---|---|---|
| C1 | ✔ | REQ-18 |
| C2 | ✔ | SRS-EP-53 |
| C3 | ✔ | buttonCount / map / connected |
| C4 | ✔ | Pick = write; cancel = 0 write. Unknown id → `off` (no crash) |
| C5 | ✔ | No logic for Infini editor |
| C6 | ✔ | Editor not gated on session |

### D. Design / Spec

| # | Status | Notes |
|---|---|---|
| D1 | ✔ | Package `pen-button-map/` (STORY-EP-056 revises as epaper-device) |
| D2 | — | Designer fills Spec scene list ⊆ this catalog |
| D3 | ✔ | No polish modal |
| D4 | ✔ | Dual-ask on SRS-EP-52 |
| D5 | ✔ | Composition in SRS-EP-52 |

### E. Consistency

| # | Status | Notes |
|---|---|---|
| E1 | ✔ | SRS-EP-52 interaction map matches |
| E2 | ✔ | Closed ids match SRS-EP-53 |
| E3 | ✔ | Historical Infini slate is not Keep |
| E4 | ✔ | No shell TabBar on epaper |

### Gap backlog

| Gap id | Check # | Artifact | Disposition | Owner |
|---|---|---|---|---|
| GAP-01 | A2 / B3 | `cta.pen_map_open` placement | Designer proposes; PM adopts. Not a missing scene | `/designer` then `/pm` |

## Done when

- Catalog + nav map + matrix complete (this file)
- Designer Spec scene list ⊆ catalog
- STORY-EP-056 parents [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor)
