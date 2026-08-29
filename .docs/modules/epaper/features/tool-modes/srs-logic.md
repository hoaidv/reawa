---
feature: tool-modes
parent_req: [REQ-03]
version: 0.2.0
lifecycle: active
---

# SRS — Tool modes Epaper (Logic)

Device-side rules for [REQ-03](../../prd.md#tool-modes).
Decision: [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1 (device-local tool state),
as amended by [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md).
What the tools act on: [SRS-EP-07](../device-document/srs-logic.md) (document),
[SRS-EP-10](../ink-box/srs-logic.md) (creation), [SRS-EP-11](../ink-box/srs-logic.md) (manipulation).

**Implementation status (code SoT, 2026-08-11):** ToolChip + `toolMode` + `stroke_begin.intent`
+ `pickables` ingest + `tool_intent` emit in `tabletcanvasitem` / `Main.qml` (STORY-EP-005).
Touch-on-chip uses MouseArea; pen-on-chip press is ignored for ink (fallback path).
**The intent-emission half of that code is superseded** — the chip and routing survive.

## [SRS-EP-04] Tool state and input routing

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Enclose-intent and Selection-intent tables retired;
     tools now invoke local document operations. Same id, content revised. -->

> **Revised 2026-08-13.** This section is now only about **what the pen does next** — tool state and
> input routing. The two intent tables are gone: there is no `intent` flag, no `pickables`, no
> `tool_intent`, and no advisory ghost. A tool invokes a local operation and the document changes
> immediately.

### Tool state

| Rule | Value |
|---|---|
| Tools | `sel_rect` \| `sel_freeform` \| `pen` |
| Recognizer toggles | `recog.ink_box` \| `recog.connector` — independent; both default **on**; dimmed (state kept) while a Selection tool is active ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md)) |
| Latch | Exclusive tool **and** both toggles latch at pen-down for the whole stroke |
| Default on launch | `pen`, both recognizers armed — the device must still be a notebook if nothing else works |
| Ownership | **Device-local UI state.** Never sent to Infini, never set by Infini (ADR-0013 §1) |
| Persistence | Not persisted across restarts in v0 |
| Input | Finger touch on the ToolChip (pen-on-chip fallback). Pen events on the chip are not ink |

### Input routing

| Tool | Pen down on canvas | Finger / pen on ToolChip | Finger on canvas |
|---|---|---|---|
| `pen` | Local ink → **dispatch** at pen-up ([ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md)): enclose / membership / connector / ordinary ink | Switch tool or flip a toggle | [SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger) / [SRS-EP-24](../region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) — not this section |
| `sel_rect` | Rect marquee / pick / move / resize against the local document ([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Same — finger canvas is REQ-10, not a fourth exclusive tool |
| `sel_freeform` | Freeform lasso / pick / move / resize ([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Same |

`pen` is the only inking exclusive tool — same Round 19 map, same paint. Recognizers differ only
in **what the device does at pen-up**, so ink latency cannot regress by toggle
([SRS-EP-01](../local-pen-ink/srs-logic.md)).

### Arming

| Rule | Value |
|---|---|
| Recognizers stay armed across strokes | Until the creator toggles them off |
| Dimmed under Selection | Armed state retained; they do not run |
| Refused enclose or connector | The stroke stays ordinary ink (or falls through per ADR-0022); no banner |

### Tool independence

Tool mode is device-local and now trivially so: the peer has no tools this campaign
([infini REQ-04](../../../infini/prd.md#smart-group) deprecated) and never sees the device's.

### Errors / partial failure

| Case | Behavior |
|---|---|
| Touch layer unavailable at runtime | Fall back to `pen` permanently; surface it in the status line; never trap the creator in a non-drawing tool |
| Session down | **All three exclusive tools and both toggles stay fully available** — editing is local ([REQ-04](../../prd.md#device-document)). Only publishing waits; the status affordance shows changes are queued |
| Pen-down starts on the ToolChip bounds | Not ink; may arm a tool when pen-on-chip fallback is active |
| Tool switched mid-gesture | The in-flight gesture completes under the tool it started with; the new tool applies from the next pen-down |

### Other logic

- **Exclusion rect = ToolChip bounds** (floating chip), not a full edge band. A stroke must never
  begin inside that rect; `InkSurface` stays full-bleed ([SRS-EP-05](./srs-ui.md), CHL-0003).
- Tool switching must not invalidate the full panel — partial refresh of the **chip** only
  (the ink area keeps its content, [SRS-EP-06](./srs-quality.md)).
- Chip anchor follows **gut orientation top** (see SRS-EP-05); exclusion rect moves with it.

<!-- revised: 2026-08-20 — PM adopted local pan ([REQ-10](../../prd.md#hand-touch)). Finger-on-canvas is SRS-EP-21 / SRS-EP-24, not a ToolChip exclusive. Viewport-follow is [SRS-EP-49](../region-sync/srs-logic.md#srs-ep-49-viewport-follow), not this file. -->

---

## [SRS-EP-23] Finger exclusive-tool switch {#srs-ep-23-finger-tool-switch}

<!-- lifecycle: active -->

**Parent:** [REQ-10](../../prd.md#hand-touch). **Links:** [SRS-EP-04](#srs-ep-04) (pen routing unchanged), [SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger).

| Rule | Value |
|---|---|
| Trigger | Finger-down that **hits a SmartGroup box** per SRS-EP-21 |
| Effect | Exclusive tool becomes `sel_freeform`; recognizer toggles dim per existing Selection rules |
| Chip | `ind.tool_active` shows freeform; p95 ≤300 ms; **chip bounds only** refresh |
| Does not | Steal ToolChip hits; switch on empty-canvas one-finger; switch on resize-knob hit (resize, no extra tool change) |
| Pen path | Unchanged — pen on a box still follows SRS-EP-11 (does not have to switch tools unless already specified there) |

QA can write: *Given `pen` active and a box at/above LOD, When finger-down inside bounds, Then tool is `sel_freeform` and chip matches, p95 ≤300 ms.*

---

## [SRS-EP-41] Barrel click vs hold-move dispatch {#srs-ep-41-barrel-dispatch}

<!-- lifecycle: active -->

**Parent:** [REQ-18](../../prd.md#pen-buttons). **Decisions:** [ADR-0025](../../../../adr/ADR-0025-barrel-vs-eraser-nib.md), [ADR-0031](../../../../adr/ADR-0031-device-settings-persist-on-epaper.md) (supersedes [ADR-0030](../../../../adr/ADR-0030-tablet-authors-pen-button-map.md)), [ADR-0034](../../../../adr/ADR-0034-erase-clip-remnants.md). **Map anatomy:** [domain/pen-button-map](../../../../domain/pen-button-map.md). **Authoring is not this section** — [SRS-EP-53](#srs-ep-53-pen-map-author). **Erase mutation is not this section** — [SRS-EP-54](../erase/srs-logic.md#srs-ep-54-erase-mode).

### Classifier

| Event | Movement vs threshold | Catalogue side | Dual-fire |
|---|---|---|---|
| Button down+up | Below threshold | **Click** | Hold-move **must not** run |
| Button down + move past threshold until release | At/above threshold | **Hold-move** | Click **must not** run on release |

Threshold is device-local (start: **12 du** of tip travel while button is down). QA fixture: 20 mixed clicks and holds → **0** events fire both. Latch map at **button-down**; on-device rebind or **device-store restore** applies to the **next** gesture.

### Click catalogue (closed — do not invent)

`toggle_pen_freeform` · `toggle_pen_eraser` · `off`

**Dropped:** `undo` (Undo stays on the ToolChip).

### Hold-move catalogue (closed — do not invent)

`temp_erase` · `drag_node_under_tip` · `off`

**Dropped:** `temp_sel_freeform` · `temp_sel_rect` (Hold-move snaps back; temporary select is meaningless if we do nothing after it).

`drag_node_under_tip`: if tip is on a hittable node at latch, move with SRS-EP-11 live-direct; if empty, **0** nodes move and **0** lasso. Combined empty→lasso/node→drag is **not** a v1 id.

### Defaults

See domain doc. 1-button Hold-move default is **`temp_erase`** (not temporary freeform). 0-button: **0** barrel gestures; ToolChip still complete. `toggle_pen_eraser` and `temp_erase` target **last-used eraser** ([SRS-EP-54](../erase/srs-logic.md#srs-ep-54-erase-mode)), via the **barrel channel**, not the nib HID flag.

### Chip during hold-move

When Hold-move is **`temp_erase`**, the chip **mirrors** that temporary tool until release, then restores unless the event was a Click toggle ([SRS-EP-42](./srs-ui.md#srs-ep-42-chip-temp-tool)). **`drag_node_under_tip` does not** switch the exclusive tool on the chip. The chip is **not** the map editor ([SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor)).

### UI-driving fields

| Field | Drives |
|---|---|
| `pen.buttonCount` | 0 / 1 / 2 slot visibility |
| `pen.map` | Live catalogue ids |
| `barrel.phase` | `idle` \| `click` \| `hold_move` |
| `toolMode` + `toolMode.restore` | Chip + routing |

---

## [SRS-EP-53] On-device pen-button map authoring and persist {#srs-ep-53-pen-map-author}

<!-- lifecycle: active -->
<!-- revised: 2026-08-20 — persist on Epaper device; drop Infini persist-up / restore-down. Same id. -->

**Parent:** [REQ-20](../../prd.md#device-settings) (persist home) · [REQ-18](../../prd.md#pen-buttons) (catalogue writes). **Decision:** [ADR-0031](../../../../adr/ADR-0031-device-settings-persist-on-epaper.md) (supersedes [ADR-0030](../../../../adr/ADR-0030-tablet-authors-pen-button-map.md)). **Anatomy:** [domain/pen-button-map](../../../../domain/pen-button-map.md). **UI:** [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor). **Scene graph:** [srs-ui-multi-scene.md](./srs-ui-multi-scene.md). **Dispatch:** [SRS-EP-41](#srs-ep-41-barrel-dispatch). **Infini peer:** [SRS-IN-23](../../../infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) **retired**. **Not the chip** — [SRS-EP-42](./srs-ui.md#srs-ep-42-chip-temp-tool).

The tablet **writes** the live map and **persists it on this device**. Infini does not author, store, or restore.

| Rule | Value |
|---|---|
| Bind | Settings page writes `buttons[].click` / `holdMove` from the closed catalogues only. New `mapId` per committed bind |
| Apply | Live map replaces immediately for the **next** gesture; in-flight latch unchanged |
| Persist | Flush to **device-local durable store** on committed bind. **0** `pen_button_map` messages. **0** `doc_*`. **0** Infini app-settings copies. **0** SVG / VectorDocument fields |
| Offline | Page remains usable. Next barrel gesture uses the live map. Persist does **not** wait on Infini |
| Restart | On Epaper start, load the device store. Next barrel gesture uses that map p95 ≤300 ms after first HID report |
| Restore-down | **0.** Ignore inbound `pen_button_map` if an old peer still emits it |
| Persist-up | **0.** Do not emit `pen_button_map` tablet→desktop |
| Capability telemetry | Optional `pen_capability` T→D on hello (HID only). Not persist |
| 0-button | Store **0** slots; ignore maps that invent indexes |
| Unknown id | Apply that slot as `off` |
| Defaults | If never bound and no device store: domain defaults |
| Other device | A different Epaper with factory defaults does **not** inherit this map from an Infini document |

### Closed ids (Settings + inline catalogues)

`cta.pen_map_open` · `cta.pen_map_close` · `nav.settings.pen_buttons` · `slot.click` · `slot.hold_move` · `list.click.toggle_pen_freeform` · `list.click.toggle_pen_eraser` · `list.click.off` · `list.hold.temp_erase` · `list.hold.drag_node_under_tip` · `list.hold.off`

`cta.pen_map_open` **placement is named** (GAP-01 adopted): leading 10 mm sibling of ToolChip. Must **not** be Infini File menu, a 5-way radio on exclusive-tool tiles, or a fourth `toolMode`.

### Routes / presentations (UI-driving)

| Closed id | Nav kind | Target `scene_id` |
|---|---|---|
| `cta.pen_map_open` | `present-modal` | `scene.pen_map_editor` |
| `cta.pen_map_close` / overlay dismiss | `dismiss` | drawing surface (underlay) |
| `list.click.*` (pick) | *(intra-scene write)* | `scene.pen_map_editor` (write live map; persist on device) |
| `list.hold.*` (pick) | *(intra-scene write)* | `scene.pen_map_editor` (write live map; persist on device) |

`slot.click` / `slot.hold_move` **do not** `present-sheet`. `scene.pen_map_click` / `scene.pen_map_hold` are **retired**. There is no list-cancel hop.

### UI-driving fields

| Field | Drives |
|---|---|
| `pen.buttonCount` | Detail layout 0 / 1 / 2; 0-button → **0** slot rows |
| `pen.map.buttons[].click` / `holdMove` | Slot values |
| `map.state` | `absent` \| `applied` |
| `session.connected` | Offline copy only — **does not** disable the page or delay persist |

QA can write: *Given Settings · Pen buttons and no session, When the creator rebinds Hold-move to drag-under-tip, Then the next hold-move uses that id, the device store holds that map, and Infini holds 0 copy.*
*Given a rebound map, When Epaper restarts on the same device, Then the next barrel gesture uses that map p95 ≤300 ms after first HID report.*

---

## [SRS-EP-44] Manual create routing {#srs-ep-44-manual-create-routing}

<!-- lifecycle: active -->

**Parent:** [REQ-17](../../prd.md#manual-create) (Should). **Links:** [SRS-EP-45](../device-document/srs-logic.md#srs-ep-45-manual-insert), [SRS-EP-46](../connector-ink/srs-logic.md#srs-ep-46-manual-connector), [SRS-EP-47](./srs-ui.md#srs-ep-47-manual-create-ui).

| Rule | Value |
|---|---|
| Entry | Closed create ids on chip **or** selection context — Designer chooses placement; **must not** add a brush/color/layer palette |
| Conflict with Pen | While a manual-place gesture is latched, pen-down on canvas **places**, it does not ink; cancel returns to `pen` |
| Ink-box enclose | Unchanged — [REQ-05](../../prd.md#device-ink-box) / SRS-EP-10. This section does not re-specify enclose |
| Cancel | Escape equivalent = second tap on the same create control, or empty-canvas tap with no drag for frame/primitive place-by-tap variants — **0** nodes committed |

Closed create kinds: `create.frame` · `create.connector` · `create.attach` · `create.primitive.ellipse` · `create.primitive.rect` · `create.primitive.line`.
