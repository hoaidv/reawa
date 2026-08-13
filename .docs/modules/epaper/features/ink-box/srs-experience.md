---
feature: ink-box
parent_req: [REQ-05, REQ-06]
version: 0.1.0
lifecycle: active
owner: pm
co_author: designer
purpose: PRD → technical bridge — journeys for on-device ink-box create and manipulate
---

# SRS — Ink-box on the device (Experience)

Start here after [REQ-05](../../prd.md#device-ink-box) / [REQ-06](../../prd.md#device-manipulation).
Policy lives in [srs-product.md](./srs-product.md); this file is how the outcomes are **lived**.

**Single scene.** Epaper has one surface — the full-bleed ink panel with a floating ToolChip
([SRS-EP-05](../tool-modes/srs-ui.md)). There are no pushed scenes, sheets, or modals, so journeys
below are **in-scene state sequences**, not navigation. Scene-graph N/A for this feature.

---

## Capability narrative

The creator is writing, not operating software. They tap `Ink-box` with a finger — the pen never
leaves the page — draw a loose rectangle around the paragraph they just wrote, and the paragraph
becomes a thing they can pick up. Later they tap `Selection`, press the box, and slide it down to
open space; the ink moves under the pen as they push it, e-ink lagging a little but never lying.
When they let go, it stays exactly where they let go.

The failure the design must design *against* is the pilot's: a box that appears a beat late from
somewhere else, or ink that settles somewhere other than where the hand released it.

---

## Entry context

| Field | Value |
|---|---|
| Persona / role | Creator drawing on the reMarkable 2, pen in hand, device in the lap or on a desk |
| Situation / when | Mid-thought on a page that already has handwriting; wants to structure or rearrange it |
| Trigger | Finger tap on `Ink-box` or `Selection` in the floating ToolChip |
| Preconditions | Epaper running fullscreen ([REQ-01](../../prd.md#local-pen-ink)); a local document exists ([REQ-04](../../prd.md#device-document)); session may be up **or down** |

---

## Primary journeys

### Journey: `journey.device_enclose` — Turn handwriting into a box

- **Realizes:** [REQ-05](../../prd.md#device-ink-box) creation A; BR-B01, BR-B03, BR-B05
- **Success end-state:** the cluster is one object with the creator's own stroke as its frame,
  visible on the panel, no peer involved

| Step | Beat (human language) | In-scene state | Notes |
|---|---|---|---|
| 1 | Creator has written a few lines and wants them to hold together | `tool.pen` default | Ink already in the local document |
| 2 | Taps `Ink-box` with a finger; the chip shows the tool is armed | `tool.ink_box.armed` | Pen never leaves the page; ≤300 ms |
| 3 | Draws a loose rectangle around the writing — it inks normally under the pen | `tool.ink_box.drawing` | Identical ink path to `Pen`; no latency cost |
| 4 | Lifts the pen; the device evaluates the enclosure and creates the box | `tool.ink_box.accepted` | ≤500 ms pen-up → visible; **0 desktop messages** |
| 5 | The frame is the stroke they drew; the writing inside is now content | `tool.ink_box.accepted` | Never a synthetic rectangle |
| 6 | The tool stays armed, so they box the next paragraph too | `tool.ink_box.armed` | Repeat without re-tapping |

### Journey: `journey.device_move` — Slide a box out of the way

- **Realizes:** [REQ-06](../../prd.md#device-manipulation); BR-B10, BR-B11, BR-B15
- **Success end-state:** the box rests exactly where the pen released it

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Taps `Selection` | `tool.selection.idle` | |
| 2 | Presses inside a box and starts dragging | `tool.selection.moving` | No prior selection needed |
| 3 | The **real ink** travels with the pen; e-ink repaints in partial passes | `tool.selection.moving` | ≥5 Hz, no full-panel flash |
| 4 | Lifts the pen | `tool.selection.selected` | Committed geometry = last previewed geometry, 0 px jump |
| 5 | Presses empty canvas to put the tool down | `tool.selection.idle` | 0 residual selection pixels |

### Journey: `journey.device_resize` — Stretch a box to fit more writing

- **Realizes:** [REQ-06](../../prd.md#device-manipulation); BR-B12, BR-B13
- **Success end-state:** bounds fit the new intent and the ink behaves per the chosen scale mode

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Presses a box without dragging — it is selected, handles appear | `tool.selection.selected` | Handles are real affordances on the panel |
| 2 | Decides whether the writing should grow with the box or keep its size | `tool.selection.mode_toggle` | `withBounds` vs `fixedInk` |
| 3 | Drags a handle; ink responds live per the chosen mode | `tool.selection.resizing` | `fixedInk`: sample size fixed, each ink keeps its own UV |
| 4 | Lifts the pen; one undo entry now covers the whole resize | `tool.selection.selected` | Boundary transformed with the frame |

### Journey: `journey.device_draw_into` — Add a line to an existing box

- **Realizes:** [REQ-05](../../prd.md#device-ink-box) draw-into; BR-B07, BR-B08

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Taps `Pen` and writes another line inside an existing box | `tool.pen` | |
| 2 | On pen-up the line joins that box as content | `tool.pen` | ≤300 ms; 0 existing content moves |
| 3 | Nothing reflows — the new line sits exactly where it was written | `tool.pen` | Free layout |

### Journey: `journey.device_select_create` — Promote a selection that already has a frame

- **Realizes:** [REQ-05](../../prd.md#device-ink-box) creation B; BR-B06

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Taps `Selection` and picks ink that includes a stroke drawn around the rest | `tool.selection.multi` | Open surround stroke is fine |
| 2 | Invokes Smart Group | `tool.selection.create_pending` | |
| 3 | The surrounding stroke becomes the frame; the rest becomes content | `tool.selection.selected` | ±1 px bounds fidelity |

---

## Critical alternate journeys

### Journey: `journey.device_enclose.alt_refused` — The enclosure does not qualify

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Creator draws a small box around nothing, or a box too small to count | `tool.ink_box.drawing` | |
| 2 | On pen-up nothing is grouped; the stroke stays as ordinary ink | `tool.ink_box.rejected` | No error banner, no modal — the ink simply is what they drew |
| 3 | They can immediately try again, or undo the stroke | `tool.ink_box.armed` | Best-effort + undo |

### Journey: `journey.device_select_create.alt_no_surround` — Nothing surrounds anything

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Creator selects scattered ink with no stroke around it and invokes Smart Group | `tool.selection.multi` | |
| 2 | Creation is refused and the reason is visible | `tool.selection.create_refused` | Selection unchanged; **no** AABB-only box |
| 3 | They draw a frame around it with `Ink-box` instead | `tool.ink_box.armed` | The refusal teaches the working path |

### Journey: `journey.device_edit.alt_offline` — Working with the link down

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | The desktop is asleep or unplugged; the chip shows the link is down | `session.disconnected` | |
| 2 | Creator encloses, moves, resizes, undoes — everything behaves normally | any tool state | 100% parity with the linked case |
| 3 | The chip shows changes are pending | `session.pending_changes` | No invisible data loss |
| 4 | The link returns; pending work publishes and the indicator clears | `session.publishing` → `session.linked` | In order, 0 lost |

### Journey: `journey.device_edit.alt_below_lod` — Zoomed too far out to manipulate

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | The desktop is zoomed out past the cutoff; boxes are tiny on the panel | `manipulation.unavailable` | |
| 2 | Creator presses a box; nothing is grabbed | `manipulation.unavailable` | 0 accidental transforms |
| 3 | The UI says manipulation is unavailable at this scale | `manipulation.unavailable` | Never silently does something else |

### Journey: `journey.device_edit.alt_undo` — Take it back

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | A box was created around the wrong strokes, or a drag went too far | any | |
| 2 | Creator undoes once | `undo.applied` | Exactly one gesture reverts |
| 3 | The document is back to the previous state, ±1 px | previous state | **No on-panel undo chrome this campaign** ([CHL-0010](../../../../../.plan/iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md) deferred); ring still ships (EP-015) |

---

## Bridge matrix

| Journey step | In-scene state | srs-ui state / control | Product rule / AC | Logic pointer |
|---|---|---|---|---|
| `journey.device_enclose` 2 | `tool.ink_box.armed` | ToolChip active-tool indicator | BR-B02 | TBD — architect |
| `journey.device_enclose` 4 | `tool.ink_box.accepted` | box visible on panel | BR-B01, BR-B03 | TBD — architect |
| `journey.device_enclose.alt_refused` 2 | `tool.ink_box.rejected` | no chrome; ink unchanged | BR-B03 | TBD — architect |
| `journey.device_move` 3 | `tool.selection.moving` | live ink under pen (no ghost) | BR-B10, BR-B15 | TBD — architect |
| `journey.device_move` 5 | `tool.selection.idle` | selection cleared, no residue | BR-B16 | TBD — architect |
| `journey.device_resize` 1 | `tool.selection.selected` | `ovl.selection_bounds`, `ovl.resize_handles` | BR-B11 | TBD — architect |
| `journey.device_resize` 2 | `tool.selection.mode_toggle` | ink-scale mode control | BR-B12 | TBD — architect |
| `journey.device_select_create.alt_no_surround` 2 | `tool.selection.create_refused` | refusal reason indicator | BR-B06 | TBD — architect |
| `journey.device_edit.alt_offline` 3 | `session.pending_changes` | status affordance on the chip | device-document BR-D11 | TBD — architect |
| `journey.device_edit.alt_below_lod` 3 | `manipulation.unavailable` | unavailable indicator | BR-B14 | TBD — architect |

---

## Anti-invent / out of journey

| Path / wish | Decision | Tracking |
|---|---|---|
| Rotate a box on the device | Defer | [REQ-08](../../prd.md#node-manipulation) |
| Multi-select / marquee on the device | Defer | [REQ-08](../../prd.md#node-manipulation) |
| Enter/exit a box to edit its children as a sub-scene | Defer | [REQ-08](../../prd.md#node-manipulation) |
| Ellipse or lasso enclosure | Reject (this campaign) | epaper PRD Non-Goals |
| Aligning writing inside the box | Reject (this campaign) | epaper PRD Non-Goals |
| A fourth tool for undo | Defer (this campaign) | [CHL-0010](../../../../../.plan/iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md) — EP-015 ships the ring with no chrome |
| Confirmation dialog before creating a box | Reject | BR-B09 — best-effort + undo, no modals on e-ink |

---

## Superseded

Replaces the desktop-side journeys in
[infini vector-document srs-experience](../../../infini/features/vector-document/srs-experience.md)
for the ink-box paths, which deprecate with [SRS-IN-10] / [SRS-IN-11] / [SRS-IN-14].
