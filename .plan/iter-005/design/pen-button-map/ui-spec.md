---
id: UI-EP-08
title: Epaper on-device pen-button map editor
parent_srs: [SRS-EP-52, SRS-EP-53, SRS-EP-42]
parent_req: [REQ-18]
stories: [STORY-EP-056]
status: draft
iter: iter-005
scenes:
  - pen-button-map-entry.html
  - pen-button-map-layout-0.html
  - pen-button-map-layout-1.html
  - pen-button-map-layout-2.html
  - pen-button-map-offline.html
  - pen-button-map-slot-click.html
  - pen-button-map-slot-hold.html
  - pen-button-map-chip-temp-erase.html
  - pen-button-map-chip-drag.html
hifi_html: pen-button-map-layout-1.html
states_showcase: pen-button-map-states.html
wireframe_html: ""
tokens: tokens.json
tokens_css: tokens.css
components: components.md
design_contract: .docs/DESIGN.md
project_tokens: .docs/design/tokens.json
project_tokens_css: .docs/design/tokens.css
project_components: .docs/design/components.md
system_components: .plan/iter-005/design/system/components
system_assets: .plan/iter-005/design/system/assets
design_index: .docs/design/index.md
fidelity: hifi
platform: epaper
---

# [UI-EP-08] — Epaper on-device pen-button map editor

Iter-local UI design for [STORY-EP-056](../../stories/STORY-EP-056.md). **One job:** bind each present barrel slot (Click, Hold-move) to exactly one closed-catalogue item **on the tablet**. HTML is a visual reference, not production Qt.

This Spec **replaces** historical Infini `[UI-IN-03]` in this package. `[UI-IN-03]` is not current. Infini [REQ-05](../../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore has **0** editor screens.

**Lock:** `.docs/DESIGN.md` and `.docs/design/index.md` are not edited this wave (SM stitches). Do not paint Infini slate, hover, Save-publish, Undo-on-Click, or temp freeform/rect.

## Source

- REQ: [REQ-18](../../../../.docs/modules/epaper/prd.md#pen-buttons) Configurable pen barrel-button accelerators
- SRS-UI editor: [SRS-EP-52](../../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor)
- SRS-Logic author: [SRS-EP-53](../../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author)
- SRS-UI chip (extra scenes, not the editor graph): [SRS-EP-42](../../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool)
- Scene graph: [srs-ui-multi-scene.md](../../../../.docs/modules/epaper/features/tool-modes/srs-ui-multi-scene.md)
- Domain: [pen-button-map](../../../../.docs/domain/pen-button-map.md)
- ADR: [ADR-0030](../../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) (supersedes ADR-0028 authoring)
- Story AC: [STORY-EP-056](../../stories/STORY-EP-056.md)
- Reference image: none (historical Infini slate in this folder is **not Keep**)
- Craft reference (read only): `viewport-follow-epaper/`, `hand-touch/`

## Experience bridge (campaign)

`epaper/tool-modes` has **no** `srs-experience.md`. Campaign override: **do not hard-stop**. Scene inventory = [SRS-EP-52](../../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) states + [srs-ui-multi-scene.md](../../../../.docs/modules/epaper/features/tool-modes/srs-ui-multi-scene.md) Keep catalog, plus REQ-18 dual-ask chip journeys from [SRS-EP-42](../../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool). Logged as Concern in the designer→SM handoff — not silently edited into `.docs/modules/**`.

## Platform profile

| Field | Value |
|---|---|
| Profile | **epaper-device** (reMarkable 2, Qt/QML fullscreen) |
| `data-platform` | `epaper` (SRS-EP-52). Mechanical `adlc gate` allowlist is still `ios\|android\|web\|desktop` — owned by existing CHL-0002, not relaxed here |
| Target frames | Landscape **246 mm × 187 mm** (1872×1404). Body 187×246 mm. Not phone chrome |
| Responsive strategy | per-target — one panel size; no reflow |
| Breakpoints / resize | N/A — fixed panel |
| Safe areas / fixed regions | ToolChip floats orientation-top **center** (UI-EP-04). `cta.pen_map_open` floats orientation-top **leading**. Overlay is an isolatable bottom rect |
| Input model | **Pen** for content; **finger** for chrome (≥10 mm / 64 du). No keyboard |
| Nav paradigm | `present-modal` hub · `present-sheet` lists · `dismiss` |
| Target minimum | Finger-eligible **10 mm × 10 mm** (64 du) |
| Density | compact 1-bit |
| Hover | **N/A** — no hover, no focus, no cursor, no motion |
| Preview | Navigator `data-preview-scale="mobile"` at **80%**. Scene files stay 246 mm × 187 mm |

**Platform kit (epaper):** press/active invert only. `:hover` and `:focus-visible` are **not designed**. Status never by color alone — invert (selected/open) vs paper (rest) vs hatch (session down / persist wait).

## Proposed entry (`cta.pen_map_open`) — GAP-01

**Placement (this design story, for PM adopt):** a lone **10 mm** 1-bit icon button in region `PenMapOpen`, floating orientation-top **leading** (`left` + `chip-inset`), **sibling of ToolChip** — same family as viewport-follow (trailing) and Undo (history cluster). Glyph: stylus with barrel dots (`icon-epaper-pen-map.svg`).

| Must not be | Why |
|---|---|
| Infini File menu | Editor is on-device ([ADR-0030](../../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md)) |
| 5-way radio on exclusive-tool tiles | Closed ToolChip inventory is three exclusives ([SRS-EP-05](../../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-05-tool-chip)) |
| Fourth exclusive `toolMode` | Map authoring is session chrome, not a tool |

Tap → `present-modal` `scene.pen_map_editor`. When the hub is up, the control is `aria-pressed="true"` (inverted). Close dismisses; live map kept.

## Screens / flow

Keep catalog = three editor scenes. Hub states are in-scene variants of `scene.pen_map_editor`. Chip journeys are **additional** REQ-18 dual-ask scenes (SRS-EP-42) in this package — not Infini, not hand-touch, not viewport-follow.

| Screen | `scene_id` | Purpose | Primary SRS |
|---|---|---|---|
| Drawing + open CTA | (drawing) | Propose `cta.pen_map_open` | SRS-EP-52 GAP-01 |
| Map editor hub | `scene.pen_map_editor` | 0/1/2 slots; offline still usable | SRS-EP-52 |
| Click list | `scene.pen_map_click` | Pick exactly one Click id | SRS-EP-52 |
| Hold-move list | `scene.pen_map_hold` | Pick exactly one Hold-move id | SRS-EP-52 |
| Chip temp erase | — (SRS-EP-42) | Chip mirrors Temporary eraser | SRS-EP-42 |
| Chip drag-under-tip | — (SRS-EP-42) | Exclusive tool does **not** switch | SRS-EP-42 |

```mermaid
flowchart LR
  draw[drawing + cta.pen_map_open] -->|present-modal| hub[scene.pen_map_editor]
  hub -->|dismiss| draw
  hub -->|slot.click present-sheet| click[scene.pen_map_click]
  hub -->|slot.hold_move present-sheet| hold[scene.pen_map_hold]
  click -->|pick or cancel dismiss| hub
  hold -->|pick or cancel dismiss| hub
```

Capability 0/1/2 is a **HID report** (`pen.buttonCount`), not a designer-invented radio.

## Layout regions

Region names **must match** HTML `data-region`. Tree extends SRS-EP-52 with justified underlay + GAP-01 entry. No third slot type.

| Region | Parent | Contents / hierarchy | Component | States |
|---|---|---|---|---|
| DeviceScreen | panel | Full panel | — | default |
| InkSurface | DeviceScreen | Full-bleed drawing underlay | — | default; hatched when overlay up |
| ToolChip | DeviceScreen | Unchanged UI-EP-04 three clusters | `.c-tool-chip` | default, armed, pressed, dimmed, queued |
| PenMapOpen | DeviceScreen | `cta.pen_map_open` — leading 10 mm tile | `.c-pen-map-open` | rest, pressed, open |
| PenMapOverlay | DeviceScreen | Editor hub — **not** inside exclusive-tool cluster | `.c-pen-map-overlay` | layout_0 / _1 / _2 / offline |
| SlotRow | PenMapOverlay | One row per **present** button. **Absent** on 0-button | `.c-pen-map-row` | present / absent |
| SlotClick | SlotRow | Closed Click value; opens Click list | `.c-pen-map-slot` | closed, selected-open |
| SlotHoldMove | SlotRow | Closed Hold-move value; opens Hold-move list | `.c-pen-map-slot` | closed, selected-open |
| ClickList | sheet over overlay | Three Click items + cancel | `.c-pen-map-list` | open |
| HoldList | sheet over overlay | Three Hold-move items + cancel | `.c-pen-map-list` | open |

**Containment:** PenMapOverlay is a child of DeviceScreen, not of ToolChip. It does not pan with the world. It is **not** SVG / VectorDocument chrome. Overlay is an isolatable bottom rect (chip-style partial refresh).

**Chrome relationship:** standout overlay on the 1-bit panel. ToolChip stays at orientation-top; overlay does not steal exclusive-tool hits.

**Gap policy:** 5 mm paper between ToolChip clusters (UI-EP-04). PenMapOpen uses panel leading inset (2 mm), not a fourth exclusive grid column.

## Closed catalogues (Designer must not add items)

Labels are **domain meanings**. Eraser **nib** is caption-only, not a slot ([ADR-0025](../../../../.docs/adr/ADR-0025-barrel-vs-eraser-nib.md)).

**Click (discrete toggle) — exactly three:**

| id | Label shown |
|---|---|
| `toggle_pen_freeform` | Current primary ↔ Freeform Select |
| `toggle_pen_eraser` | Current primary ↔ Eraser |
| `off` | Off |

**Not in v1:** `undo`.

**Hold-move (temporary while held and moving) — exactly three:**

| id | Label shown |
|---|---|
| `temp_erase` | Temporary eraser |
| `drag_node_under_tip` | Drag node under tip |
| `off` | Off |

**Not in v1:** `temp_sel_freeform`, `temp_sel_rect`.

**Defaults** ([domain](../../../../.docs/domain/pen-button-map.md#defaults)):

| Capability | Button 1 | Button 2 |
|---|---|---|
| 1-button | Click `toggle_pen_freeform`, Hold-move `temp_erase` | — |
| 2-button | Same as 1-button | Click `toggle_pen_eraser`, Hold-move `temp_erase` |
| 0-button | **0** slot rows | — |

## Closed control inventory

| id | Kind | Notes |
|---|---|---|
| `cta.pen_map_open` | entry | Leading 10 mm tile (proposal) |
| `cta.pen_map_close` | dismiss | Overlay header; live map kept |
| `slot.click` | value + open list | Per present button; **absent** on 0-button |
| `slot.hold_move` | value + open list | Per present button; **absent** on 0-button |
| `list.click.toggle_pen_freeform` | pick | Write + dismiss hub |
| `list.click.toggle_pen_eraser` | pick | Write + dismiss hub |
| `list.click.off` | pick | Write + dismiss hub |
| `list.hold.temp_erase` | pick | Write + dismiss hub |
| `list.hold.drag_node_under_tip` | pick | Write + dismiss hub |
| `list.hold.off` | pick | Write + dismiss hub |
| `list.cancel` | dismiss | 0 writes |

Designer **must not** add `undo`, `temp_sel_freeform`, `temp_sel_rect`, an eraser-nib slot, or a Save/publish footer.

## Component inventory

Closed list. Detail in [`components.md`](./components.md).

| Component | Kind | Source | Pattern id / CSS class | Variant / props | Used in regions |
|---|---|---|---|---|---|
| PenMapOpen | screen | build | `.c-pen-map-open` `.c-pen-map-open-btn` | 10 mm leading cluster | PenMapOpen |
| PenMapOverlay | screen | build | `.c-pen-map-overlay` | 0/1/2 / offline | PenMapOverlay |
| PenMapSlotRow | screen | build | `.c-pen-map-row` | index 1 \| 2 | SlotRow |
| PenMapSlot | screen | build | `.c-pen-map-slot` | click \| hold | SlotClick, SlotHoldMove |
| PenMapList | screen | build | `.c-pen-map-list` `.c-pen-map-option` | click \| hold | ClickList, HoldList |
| ToolChip | screen | reuse UI-EP-04 | `.c-tool-chip` | 3 clusters · 10 mm | ToolChip |

No Save button. Historical Infini `.c-pen-map-btn` / hover select are **not** this package.

## Tokens used

Closed 1-bit set — [`tokens.json`](./tokens.json) / [`tokens.css`](./tokens.css). Paper/ink only. Does **not** copy Infini teal/slate. DESIGN.md desktop system is **advisory only**.

## Design system readiness

| Check | Evidence |
|---|---|
| DESIGN.md reconciled | Advisory 1-bit paper/ink; Infini hover/slate **not transferred**. File not edited (lock) |
| Project tokens | Package subset: paper, ink, 10 mm targets, 0 radius |
| tokens.css generated | package `tokens.css` |
| Component catalog complete | six rows → self-contained `.html` |
| Pattern-only reuse | scenes copy `.c-pen-map-*` + `.c-tool-chip` |
| Icons | Unique `icon-epaper-pen-map-*` 1-bit SVGs — **not** Infini `icon-pen-map-*` |

## States (required)

| State id | Scene | File | When |
|---|---|---|---|
| (drawing entry) | drawing | `pen-button-map-entry.html` | CTA visible; overlay closed |
| `map.layout_0` | `scene.pen_map_editor` | `pen-button-map-layout-0.html` | 0-button: **0** slot rows |
| `map.layout_1` | `scene.pen_map_editor` | `pen-button-map-layout-1.html` | 1-button defaults (**hifi primary**) |
| `map.layout_2` | `scene.pen_map_editor` | `pen-button-map-layout-2.html` | 2-button defaults |
| `map.offline` | `scene.pen_map_editor` | `pen-button-map-offline.html` | Session down; editor usable; persist waits |
| `map.slot_click` | `scene.pen_map_click` | `pen-button-map-slot-click.html` | Click list — three items only |
| `map.slot_hold` | `scene.pen_map_hold` | `pen-button-map-slot-hold.html` | Hold-move list — three items only |
| `map.rebound_next_gesture` | underlay caption on hub | (in layout-1 after pick) | Bind committed; in-flight unchanged |
| `barrel.hold_temp_erase` | SRS-EP-42 | `pen-button-map-chip-temp-erase.html` | Chip **mirrors** Temporary eraser |
| `barrel.hold_drag_under_tip` | SRS-EP-42 | `pen-button-map-chip-drag.html` | Exclusive tool **does not** switch |

**Dropped Infini journeys (do not paint):** `map.invalid_stale` desktop editor; `map.offline_then_publish` Save-locally-then-publish; D9 four-item Click; D9 five-item Hold-move; hover/focus-ring slate.

## Control states & reactivity (required)

Touch profile: **no hover, no focus**. Press = invert. Proof: `pen-button-map-states.html`.

| Control | hover* | focus-visible | active/press | disabled | selected | Showcase |
|---|---|---|---|---|---|---|
| `cta.pen_map_open` | N/A | N/A | ✓ invert | — | open = inverted | `pen-button-map-states.html` |
| `cta.pen_map_close` | N/A | N/A | ✓ invert | — | — | `pen-button-map-states.html` |
| `slot.click` / `slot.hold_move` | N/A | N/A | ✓ invert | absent on 0-button | — | `pen-button-map-states.html` |
| `.c-pen-map-option` | N/A | N/A | ✓ invert | — | `aria-selected` invert | `pen-button-map-states.html` |
| `.c-tool-btn` | N/A | N/A | ✓ invert | dimmed hatch | `aria-pressed` invert | `pen-button-map-states.html` |

\* Epaper: hover N/A. No-op still inverts on press.

## Interaction map

| Control | Action | Destination | Side-effect | Feedback | Nav kind |
|---|---|---|---|---|---|
| `cta.pen_map_open` | tap | `scene.pen_map_editor` | overlay up | press invert | `present-modal` |
| `cta.pen_map_close` / empty overlay | tap | drawing | overlay gone; live map kept | press invert | `dismiss` |
| `slot.click` | tap | `scene.pen_map_click` | Click sheet | press invert | `present-sheet` |
| `slot.hold_move` | tap | `scene.pen_map_hold` | Hold-move sheet | press invert | `present-sheet` |
| `list.click.*` | tap | `scene.pen_map_editor` | write Click id; persist-up if linked | selected invert | `dismiss` |
| `list.hold.*` | tap | `scene.pen_map_editor` | write Hold-move id | selected invert | `dismiss` |
| cancel | tap | `scene.pen_map_editor` | **0** writes | press invert | `dismiss` |

## Interaction & a11y

- No keyboard, no focus ring, no cursor (`cursor: none`)
- Finger targets ≥10 mm; list rows min-height 10 mm
- Selected / open / persist-wait never by color alone (invert or hatch + text)
- Heading: overlay `h1` Pen-button map; `h2` Button n
- Icons decorative `alt=""` when adjacent text exists; CTA has `aria-label`
- Lists: three options only; Click never contains Hold-move ids (except shared `off`)
- 0-button: 0 slot controls (not disabled fakes)
- Motion: **none**. `prefers-reduced-motion` is already none
- Contrast: ink on paper (1-bit)

## Copy

| Element | Copy | Source |
|---|---|---|
| Overlay title | Pen-button map | REQ-18 |
| Close | Close | `cta.pen_map_close` |
| Capability 0 | Tablet reports 0 barrel buttons. Slots are hidden. | SRS-EP-52 `map.layout_0` |
| Capability 1 | Tablet reports 1 barrel button. | `pen.buttonCount` |
| Capability 2 | Tablet reports 2 barrel buttons. | `pen.buttonCount` |
| Row | Button 1 / Button 2 | domain index |
| Slot labels | Click / Hold-move | glossary |
| Nib caption | Eraser nib is hardware, not a barrel slot. | ADR-0025 |
| Offline | Desktop session down. Live map still applies. Persist waits. | SRS-EP-53 offline |
| Rebound | Next gesture uses this map. In-flight unchanged. | `map.rebound_next_gesture` |
| Click group | Click | closed catalogue |
| Hold group | Hold-move | closed catalogue |
| Cancel | Cancel | 0 write |
| Open CTA | Pen buttons | GAP-01 `aria-label` |
| Chip temp erase | Hold-move Temporary eraser — chip mirrors; release restores | SRS-EP-42 |
| Chip drag | Hold-move Drag node under tip — exclusive tool unchanged | SRS-EP-42 |

## Trace matrix

| Region / state | SRS | Story AC | Notes |
|---|---|---|---|
| PenMapOverlay / layouts | SRS-EP-52 | 0/1/2 capability | not Infini settings |
| SlotClick catalogue | SRS-EP-52 / SRS-EP-53 | three Click items; 0 Undo | closed |
| SlotHoldMove catalogue | SRS-EP-52 / SRS-EP-53 | three Hold-move; 0 temp freeform/rect | closed |
| 0-button absent slots | SRS-EP-52 / SRS-EP-43 | 0 fake bindings | layout_0 |
| Offline editor | SRS-EP-53 | usable; persist waits | no Save |
| Entry CTA | SRS-EP-52 GAP-01 | on-device; not File menu / 5-way | leading tile |
| Chip temp erase | SRS-EP-42 | mirror then restore | this package |
| Chip drag | SRS-EP-42 | exclusive does not switch | this package |
| `data-platform: epaper` | SRS-EP-52 | 1-bit, no hover | all scenes |

## SRS delta table (mandatory after HTML)

Re-read [SRS-EP-52](../../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) + [srs-ui-multi-scene.md](../../../../.docs/modules/epaper/features/tool-modes/srs-ui-multi-scene.md) after paint.

| SRS item | Design (hi-fi) | Result |
|---|---|---|
| Purpose: bind present slots on tablet | PenMapOverlay + lists | match |
| Click catalogue 3 ids | `map.slot_click` | match — no Undo |
| Hold-move catalogue 3 ids | `map.slot_hold` | match — no temp freeform/rect |
| Region DeviceScreen | `data-region="DeviceScreen"` | match |
| Region PenMapOverlay | `data-region="PenMapOverlay"` | match |
| Region SlotRow | `data-region="SlotRow"` | match |
| Region SlotClick | `data-region="SlotClick"` | match |
| Region SlotHoldMove | `data-region="SlotHoldMove"` | match |
| Region ClickList | `data-region="ClickList"` | match |
| Region HoldList | `data-region="HoldList"` | match |
| 0-button 0 rows | layout_0 omits SlotRow | match |
| `data-platform: epaper` | html + DeviceScreen | match |
| No hover / focus / cursor / motion | press invert only | match |
| Targets ≥64 du / 10 mm | `--target-epaper: 10mm` | match |
| States layout_0/1/2 / offline | four hub files | match |
| States slot_click / slot_hold | two list files | match |
| `cta.pen_map_open` placement | leading PenMapOpen | **propose** (GAP-01) |
| Chip hold-move (SRS-EP-42) | two extra scenes | match dual-ask — not in editor graph |
| Infini desktop chrome | none | match — dropped |
| Save-publish footer | none | match |
| No third slot type | Click + Hold-move only | match |
| Eraser nib not a slot | caption only | match |
| Editor not gated on session | offline hub still interactive | match |
| srs-experience journeys | missing file | **omit + Concern** (campaign override) |
| Composition layers | SRS-EP-52 + underlay/entry | match + justified PenMapOpen / ToolChip / InkSurface |
| Nav kinds present-modal / present-sheet / dismiss | relative hops | match |
| Scene graph Keep ⊆ catalog | three editor scenes | match |
| Extra chip scenes | SRS-EP-42 dual-ask | **in package** — not undeclared popups |

## Scenes (N scenarios → N self-contained HTML files)

| Scenario | File | Complex folder? | Notes |
|---|---|---|---|
| drawing + open CTA | `pen-button-map-entry.html` | no | hop → layout-1 |
| `map.layout_0` | `pen-button-map-layout-0.html` | no | 0 rows; close → entry |
| `map.layout_1` | `pen-button-map-layout-1.html` | no | **hifi primary** |
| `map.layout_2` | `pen-button-map-layout-2.html` | no | two rows |
| `map.offline` | `pen-button-map-offline.html` | no | persist waits; slots still open |
| `map.slot_click` | `pen-button-map-slot-click.html` | no | three Click + cancel |
| `map.slot_hold` | `pen-button-map-slot-hold.html` | no | three Hold-move + cancel |
| `barrel.hold_temp_erase` | `pen-button-map-chip-temp-erase.html` | no | chip mirrors eraser |
| `barrel.hold_drag_under_tip` | `pen-button-map-chip-drag.html` | no | Pen stays armed |
| states showcase | `pen-button-map-states.html` | no | not a scenario |

**Product hops (relative):**

| From | Control | To | Nav kind |
|---|---|---|---|
| entry | `cta.pen_map_open` | `pen-button-map-layout-1.html` | `present-modal` |
| layout-* / offline | `cta.pen_map_close` | `pen-button-map-entry.html` | `dismiss` |
| layout-1 / offline / layout-2 | `slot.click` | `pen-button-map-slot-click.html` | `present-sheet` |
| layout-1 / offline / layout-2 | `slot.hold_move` | `pen-button-map-slot-hold.html` | `present-sheet` |
| slot-click pick | `list.click.*` | `pen-button-map-layout-1.html` | `dismiss` (write) |
| slot-click cancel | cancel | `pen-button-map-layout-1.html` | `dismiss` (0 write) |
| slot-hold pick | `list.hold.*` | `pen-button-map-layout-1.html` | `dismiss` (write) |
| slot-hold cancel | cancel | `pen-button-map-layout-1.html` | `dismiss` (0 write) |
| layout-0 | no slots | — | close only |

**Deleted (obsolete Infini):** `pen-button-map-invalid-stale.html`, `pen-button-map-offline-then-publish.html`.

## Icons / assets

UI icons are **new 1-bit** SVG under `../system/assets/` with unique `icon-epaper-pen-map-*` names. Do **not** reuse Infini `icon-pen-map-*` (hover/`currentColor`) as the 1-bit truth. ToolChip compose reuses existing `icon-epaper-*` exclusives.

| Icon | Kind | File | Used in |
|---|---|---|---|
| Open CTA (stylus + barrels) | system | `../system/assets/icon-epaper-pen-map.svg` | PenMapOpen |
| Close | system | `../system/assets/icon-epaper-pen-map-close.svg` | overlay header |
| Click slot | system | `../system/assets/icon-epaper-pen-map-click.svg` | SlotClick |
| Hold-move slot | system | `../system/assets/icon-epaper-pen-map-hold.svg` | SlotHoldMove |
| Button 1 | system | `../system/assets/icon-epaper-pen-map-button-1.svg` | SlotRow |
| Button 2 | system | `../system/assets/icon-epaper-pen-map-button-2.svg` | SlotRow |
| Off | system | `../system/assets/icon-epaper-pen-map-off.svg` | `off` options |
| Eraser (barrel, not nib) | system | `../system/assets/icon-epaper-pen-map-eraser.svg` | eraser catalogue + chip mirror |
| Freeform | system | `../system/assets/icon-epaper-pen-map-freeform.svg` | Click freeform item |
| Drag | system | `../system/assets/icon-epaper-pen-map-drag.svg` | `drag_node_under_tip` |
| Offline / persist wait | system | `../system/assets/icon-epaper-pen-map-offline.svg` | overlay offline caption |

Chip compose (existing, do not duplicate): `icon-epaper-pen.svg`, `icon-epaper-sel-freeform.svg`, `icon-epaper-sel-rect.svg`, `icon-epaper-recog-ink-box.svg`, `icon-epaper-recog-connector.svg`, `icon-epaper-undo.svg`, `icon-epaper-redo.svg`.

## Self-contained HTML components

| Name | Kind | File path | Variants demoed |
|---|---|---|---|
| PenMapOpen | screen | `./components/pen-map-open.html` | rest, pressed, open |
| PenMapOverlay | screen | `./components/pen-map-overlay.html` | 0/1/2 + offline |
| PenMapSlotRow | screen | `./components/pen-map-slot-row.html` | index 1 and 2 |
| PenMapList | screen | `./components/pen-map-list.html` | Click 3 / Hold 3; selected; cancel |
| ToolChip | screen | `./components/tool-chip.html` | UI-EP-04 compose; temp-erase mirror |

## Quality evidence

| Check | Evidence / result |
|---|---|
| Existing system discovered | UI-EP-04 / UI-EP-07 1-bit kit; Infini pen-map-* **not** reused |
| Required platform frames | landscape 246×187 mm; navigator 80% |
| Component/state coverage | inventory + states showcase |
| Structural audit | `data-region` tree; `var(--…)` bindings |
| Accessibility audit | labels, 10 mm targets, invert not color |
| Content resilience | long catalogue labels wrap; 0-button empty guidance |

## Open questions

- `cta.pen_map_open` leading-tile placement is a **proposal** (GAP-01) — PM adopts.
- `srs-experience.md` missing for tool-modes — campaign Concern, not a CHL (do not edit `.docs/modules/**`).

## Gate checklist

See `html-ui-quality.md` and `ui-spec-gate.md`. Index row in `.docs/design/index.md` deferred to SM (lock). Experience product file: campaign override, Concern in handoff.
