---
feature: tool-modes
parent_req: [REQ-20, REQ-18]
version: 0.2.0
lifecycle: active
purpose: Multi-scene UX for Device Settings · Pen buttons — one Settings page; catalogues inline
---

# SRS — Device Settings · Pen buttons (Scene graph)

Co-authors: `/pm` (REQ-20 / REQ-18 journeys) · `/architect` (routes below) · `/designer` (composition per scene). Chip hold-move mirror is **[SRS-EP-42](./srs-ui.md#srs-ep-42-chip-temp-tool)** — not in this graph.

Platform: **epaper-device** (`data-platform: epaper`). No hover, no keyboard, no deep links.

## Navigation vocabulary (binding)

| Kind | Meaning | Typical chrome |
|---|---|---|
| `present-modal` | Full-panel Settings over the drawing surface | Parent covered; ToolChip policy is Designer craft |
| `dismiss` | Close Settings | Drawing resumes |
| `system` | N/A this graph | No OS picker |

**No `present-sheet`.** [CHL-0025](../../../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md) adopted: Click / Hold-move catalogues are **intra-scene** (inline radios). No `push` / `tab-switch` / `deep-link` on this surface.

## Scene catalog

### Scene: `scene.pen_map_editor` — Device Settings · Pen buttons

| Field | Value |
|---|---|
| scene_id | `scene.pen_map_editor` |
| Purpose (one job) | Master-detail Settings: Pen buttons selected; bind present barrel slots inline |
| Presentation | `present-modal` |
| Platform container | Epaper full-panel Settings on DeviceScreen (1-bit) |
| Primary `srs-ui` | [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor) |
| Design package slug | `pen-button-map` |
| Entry points | Drawing chrome `cta.pen_map_open` (GAP-01 **adopted**: leading 10 mm sibling of ToolChip) |
| Exit points | `cta.pen_map_close` / overlay dismiss → drawing |
| States covered | `map.entry` (drawing) · `map.layout_0` · `map.layout_1` · `map.layout_2` · `map.offline` · `map.rebound_next_gesture` |
| Logic binding | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) — write live map; persist **on this device** |
| Parent REQ | [REQ-20](../../prd.md#device-settings) (shell, entry, persist) · [REQ-18](../../prd.md#pen-buttons) (catalogues) |
| Out of scope | Infini desktop settings; 5-way radio on exclusive-tool tiles; chip temp-tool; other Settings master items; sheets |

### Scene: `scene.pen_map_click` — Click catalogue list (retired)

<!-- lifecycle: retired -->
<!-- superseded-by: scene.pen_map_editor -->

**Do not design. Do not implement.** Click catalogue is **inline** in `scene.pen_map_editor` ([CHL-0025](../../../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md)). Id kept.

| Field | Value |
|---|---|
| scene_id | `scene.pen_map_click` |
| Purpose (historical) | Pick exactly one closed Click id on a sheet |
| Presentation | `present-sheet` — **withdrawn** |
| Parent REQ | [REQ-18](../../prd.md#pen-buttons) |

### Scene: `scene.pen_map_hold` — Hold-move catalogue list (retired)

<!-- lifecycle: retired -->
<!-- superseded-by: scene.pen_map_editor -->

**Do not design. Do not implement.** Hold-move catalogue is **inline** in `scene.pen_map_editor` ([CHL-0025](../../../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md)). Id kept.

| Field | Value |
|---|---|
| scene_id | `scene.pen_map_hold` |
| Purpose (historical) | Pick exactly one closed Hold-move id on a sheet |
| Presentation | `present-sheet` — **withdrawn** |
| Parent REQ | [REQ-18](../../prd.md#pen-buttons) |

## Inter-scene navigation map

| From scene | Control / trigger | Nav kind | To scene | Presentation | Chrome side-effect | Logic / REQ |
|---|---|---|---|---|---|---|
| (drawing) | `cta.pen_map_open` | `present-modal` | `scene.pen_map_editor` | Settings page | Exclusive tool unchanged; page usable if session down | [SRS-EP-53] · [REQ-20] |
| `scene.pen_map_editor` | `cta.pen_map_close` / dismiss | `dismiss` | (drawing) | dismiss | Overlay gone; live map kept | no revert on close |
| `scene.pen_map_editor` | `list.click.*` / `list.hold.*` | *(intra-scene)* | `scene.pen_map_editor` | in place | Detail shows new value | write live map; persist on device |

0-button hub: `slot.click` / `slot.hold_move` **absent** — no catalogue rows (A3 still holds via `cta.pen_map_close`). **0** hops to retired sheet scenes.

## Intra-scene vs inter-scene

| Scope | Where it lives |
|---|---|
| Slot value display, 0/1/2 layout, offline copy, inline catalogues | [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor) |
| Open/close Settings | **This** navigation map |
| Catalogue ids + device persist | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) |

## Reference / JPEG → scene

No JPEG pack. Historical Infini slate and sheet scenes in `pen-button-map/` are **not** Keep — Designer Settings page is Keep.

## States not in any JPEG (still required)

| Needed state / scene | Why | id |
|---|---|---|
| Leading entry tile | GAP-01 adopted | `map.entry` |
| 0-button empty detail | PRD: 0 fake slots | `map.layout_0` |
| Session down | Live map + page must work; persist does not wait | `map.offline` |

Chip hold-move states are **not** JPEG gaps here — they belong to SRS-EP-42.

## Traceability matrix

| scene_id | `[REQ-NN]` | `srs-logic` | UI-driving fields | `srs-ui` / design slug |
|---|---|---|---|---|
| `scene.pen_map_editor` | [REQ-20](../../prd.md#device-settings) · [REQ-18](../../prd.md#pen-buttons) | [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author) | `pen.buttonCount`, `pen.map`, `session.connected` | SRS-EP-52 · `pen-button-map` |
| `scene.pen_map_click` | — | **retired** | — | do not paint |
| `scene.pen_map_hold` | — | **retired** | — | do not paint |

## Orphan / gap audit

### A. Reachability

| # | Status | Notes |
|---|---|---|
| A1 | ✔ | Settings from `cta.pen_map_open`; catalogues inline (no sheet hop) |
| A2 | ✔ | GAP-01 **adopted** — leading 10 mm tile is the in-scope start |
| A3 | ✔ | Close on Settings; in-place pick has no cancel hop |
| A4 | ✔ | `dismiss` target is drawing |
| A5 | ✔ | N/A — no deep links on epaper-device |

### B. Presentation / interaction

| # | Status | Notes |
|---|---|---|
| B1 | ✔ | Kinds explicit; `present-sheet` withdrawn |
| B2 | ✔ | Full-panel Settings on epaper-device |
| B3 | ✔ | Leading tile is sibling of ToolChip, not a fourth exclusive |
| B4 | ✔ | Inventory in SRS-EP-52 |
| B5 | ✔ | Catalogues presented inline in DetailPane |
| B6 | ✔ | Pickers owned by `slot.click` / `slot.hold_move` (intra-scene) |

### C. Logic / REQ

| # | Status | Notes |
|---|---|---|
| C1 | ✔ | REQ-20 shell/persist · REQ-18 catalogues |
| C2 | ✔ | SRS-EP-53 |
| C3 | ✔ | buttonCount / map / connected (connected does not gate persist) |
| C4 | ✔ | Pick = write in place. Unknown id → `off` (no crash) |
| C5 | ✔ | No logic for Infini editor or Infini persist |
| C6 | ✔ | Page not gated on session |

### D. Design / Spec

| # | Status | Notes |
|---|---|---|
| D1 | ✔ | Package `pen-button-map/` (Settings page; STORY-EP-056) |
| D2 | — | Designer Spec scene list ⊆ this catalog (Keep = `scene.pen_map_editor` only) |
| D3 | ✔ | No polish modal |
| D4 | ✔ | Dual-ask on SRS-EP-52 |
| D5 | ✔ | Composition in SRS-EP-52 |

### E. Consistency

| # | Status | Notes |
|---|---|---|
| E1 | ✔ | SRS-EP-52 interaction map matches (0 sheets) |
| E2 | ✔ | Closed ids match SRS-EP-53 |
| E3 | ✔ | Historical Infini slate / sheet scenes are not Keep |
| E4 | ✔ | No shell TabBar on epaper |

### Gap backlog

| Gap id | Check # | Artifact | Disposition | Owner |
|---|---|---|---|---|
| GAP-01 | A2 / B3 | `cta.pen_map_open` placement | **Adopted** 2026-08-20 — leading 10 mm sibling of ToolChip | `/pm` (locked) · `/architect` (this rebind) |

## Done when

- Catalog + nav map + matrix complete (this file)
- Keep = `scene.pen_map_editor` only
- Designer Spec scene list ⊆ catalog
- STORY-EP-056 / STORY-EP-058 parent [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor)
