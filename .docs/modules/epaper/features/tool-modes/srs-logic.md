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
| `pen` | Local ink → **dispatch** at pen-up ([ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md)): enclose / membership / connector / ordinary ink | Switch tool or flip a toggle | Ignored (no on-device pan — PRD Non-Goal) |
| `sel_rect` | Rect marquee / pick / move / resize against the local document ([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Ignored |
| `sel_freeform` | Freeform lasso / pick / move / resize ([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Ignored |

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

<!-- CHL-0022: the Input routing row “Finger on canvas | Ignored (no on-device pan)” is **not** the parent of [REQ-10](../../prd.md#hand-touch). Implement [SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger) / [SRS-EP-24](../region-sync/srs-logic.md#srs-ep-24-two-finger-viewport). Do not silently rewrite the table above until PM adopts CHL-0022. -->

---

## [SRS-EP-23] Finger exclusive-tool switch {#srs-ep-23-finger-tool-switch}

<!-- lifecycle: active -->

**Parent:** [REQ-10](../../prd.md#hand-touch). **Links:** [SRS-EP-04](#srs-ep-04) (pen routing unchanged), [SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger).

| Rule | Value |
|---|---|
| Trigger | Finger-down that **hits a SmartGroup box** per SRS-EP-21 |
| Effect | Exclusive tool becomes `sel_freeform`; recognizer toggles dim per existing Selection rules |
| Chip | `ind.tool_active` shows freeform; p95 ≤300 ms; **chip bounds only** refresh |
| Does not | Steal ToolChip hits; switch on empty-canvas one-finger; switch on &lt;64 du anchors |
| Pen path | Unchanged — pen on a box still follows SRS-EP-11 (does not have to switch tools unless already specified there) |

QA can write: *Given `pen` active and a box at/above LOD, When finger-down inside bounds, Then tool is `sel_freeform` and chip matches, p95 ≤300 ms.*

---

## [SRS-EP-41] Barrel click vs hold-move dispatch {#srs-ep-41-barrel-dispatch}

<!-- lifecycle: active -->

**Parent:** [REQ-18](../../prd.md#pen-buttons). **Decisions:** [ADR-0025](../../../../adr/ADR-0025-barrel-vs-eraser-nib.md), [ADR-0028](../../../../adr/ADR-0028-pen-button-map-settings-channel.md). **Map anatomy:** [domain/pen-button-map](../../../../domain/pen-button-map.md). **Nib erase is not this section** — [SRS-EP-27](../local-pen-ink/srs-logic.md#srs-ep-27-eraser-nib).

### Classifier

| Event | Movement vs threshold | Catalogue side | Dual-fire |
|---|---|---|---|
| Button down+up | Below threshold | **Click** | Hold-move **must not** run |
| Button down + move past threshold until release | At/above threshold | **Hold-move** | Click **must not** run on release |

Threshold is device-local (start: **12 du** of tip travel while button is down). QA fixture: 20 mixed clicks and holds → **0** events fire both. Latch map at **button-down**; Infini rebind applies to the **next** gesture.

### Click catalogue (closed — do not invent)

`toggle_pen_freeform` · `toggle_pen_eraser` · `undo` · `off`

### Hold-move catalogue (closed — do not invent)

`temp_sel_freeform` · `temp_sel_rect` · `temp_erase` · `drag_node_under_tip` · `off`

`drag_node_under_tip`: if tip is on a hittable node at latch, move with SRS-EP-11 live-direct; if empty, **0** nodes move and **0** lasso. Combined empty→lasso/node→drag is **not** a v1 id.

### Defaults

See domain doc. 0-button: **0** barrel gestures; ToolChip still complete. 2-button B2 hold-move `temp_erase` uses Path A **mutation** ([SRS-EP-27](../local-pen-ink/srs-logic.md#srs-ep-27-eraser-nib)) via the **barrel channel**, not the nib HID flag.

### Chip during hold-move

Shows the **temporary** exclusive tool (or erase-in-progress) until release, then restores unless the event was a Click toggle ([SRS-EP-42](./srs-ui.md#srs-ep-42-chip-temp-tool)).

### UI-driving fields

| Field | Drives |
|---|---|
| `pen.buttonCount` | 0 / 1 / 2 slot visibility |
| `pen.map` | Live catalogue ids |
| `barrel.phase` | `idle` \| `click` \| `hold_move` |
| `toolMode` + `toolMode.restore` | Chip + routing |

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
