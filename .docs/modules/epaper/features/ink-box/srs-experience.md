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
| Trigger | Finger tap on `Pen` / a Selection tool / a recognizer toggle in the floating ToolChip |
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
| 2 | Confirms **Ink-box recognition** is armed (ships on; one tap if they had turned it off) | `recog.ink_box.on` | Pen never leaves the page; ≤300 ms |
| 3 | Draws a loose rectangle around the writing — it inks normally under the pen | `tool.pen` | Identical ink path; no latency cost |
| 4 | Lifts the pen; the device evaluates the enclosure and creates the box | `recog.rejected` if guards fail, else box visible | ≤500 ms pen-up → visible; **0 desktop messages** |
| 5 | The frame is the stroke they drew; the writing inside is now content | `tool.pen` | Never a synthetic rectangle |
| 6 | The toggle stays armed, so they box the next paragraph too | `recog.ink_box.on` | Repeat without re-tapping |

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
- **Chrome:** [ADR-0016](../../../../adr/ADR-0016-selection-create-enclose-cta.md)

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Taps **Selection rect** or **Selection freeform** on the primary chip (3 tools + 2 toggles) | `sel.none` | [ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md) |
| 2a | Rect armed: one straight drag — thin dotted **rectangle** follows tip | `sel.marquee` | AABB membership |
| 2b | Freeform armed: draw around — thin dotted **polyline**; pen-up closes it | `sel.lasso` | Inside-polyline membership |
| 3 | Pen-up — dotted rect **tightly** around selected nodes’ AABBs + 6 anchors; icon-only Enclose | `sel.nodes_selected` | 0 extra padding; lasso chrome gone |
| 4 | Taps **Enclose** | `tool.selection.create_pending` | Explicit CTA — no silent invent |
| 5 | Surround stroke becomes the frame; the rest becomes content | `sel.selected` | ±1 px bounds fidelity |

### Journey: `journey.device_nested_tap` — Pick the inner box

- **Realizes:** [REQ-06](../../prd.md#device-manipulation); BR-B23
- **Success end-state:** the inner box is selected; chrome and toolbar are the child’s; parent pose unchanged

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | An ink-box sits inside another (paste or enclose) | `sel.none` | Child paints correctly after camera change, **inside** the parent AABB |
| 2 | Taps the inner box | `sel.selected` (child) | Child wins hit-test where AABBs overlap; overflow is not hittable |
| 3 | Drags or resizes | `sel.moving` / `sel.resizing.*` | Own-transform only |
| 4 | Releases; if ≥80% of natural area is in a new container, parent changes | `sel.selected` | Rule 5 at commit, not during drag |

### Journey: `journey.device_enclose_flatten` — Letter that became a box joins the paragraph

- **Realizes:** [REQ-05](../../prd.md#device-ink-box); BR-B21
- **Success end-state:** the letter ink is content of the new box; no empty wrapper left behind

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | A letter (O, D, …) was recognized as an empty ink-box | `tool.pen` | Boundary only |
| 2 | Creator encloses the paragraph (Creation A or Enclose CTA) | enclose commit | Captures the empty box |
| 3 | The letter is ordinary content ink of the new box | `tool.pen` | Flatten; 0 leftover empty layer |
| 4 | Moving the paragraph takes the letter with it | `sel.moving` | The original defect is gone |

---

## Critical alternate journeys

### Journey: `journey.device_enclose.alt_refused` — The enclosure does not qualify

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Creator draws a small box around nothing, or a box too small to count | `tool.pen` | `recog.ink_box` armed |
| 2 | On pen-up nothing is grouped; the stroke stays as ordinary ink | `recog.rejected` | No error banner, no modal — the ink simply is what they drew |
| 3 | They can immediately try again, or undo the stroke | `recog.ink_box.on` | Best-effort + undo |

### Journey: `journey.device_select_create.alt_no_surround` — Nothing surrounds anything

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Creator rubber-band selects scattered ink with no stroke around it and taps **Enclose** | `sel.nodes_selected` | |
| 2 | Creation is refused and the reason is visible | `sel.create_refused` | Selection unchanged; **no** AABB-only box |
| 3 | They draw a frame around it with `Pen` and Ink-box recognition armed instead | `recog.ink_box.on` | The refusal teaches the working path |

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
| 3 | The document is back to the previous state, ±1 px | previous state | Tap `cta.undo` on the primary strip ([ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md)); `cta.redo` restores if no newer commit |

---

## Bridge matrix

| Journey step | In-scene state | srs-ui state / control | Product rule / AC | Logic pointer |
|---|---|---|---|---|
| `journey.device_enclose` 2 | `recog.ink_box.on` | `tgl.recog.ink_box` armed | BR-B02 | [SRS-EP-04](../tool-modes/srs-logic.md) |
| `journey.device_enclose` 4 | box visible on panel | enclose commit | BR-B01, BR-B03 | [SRS-EP-10](./srs-logic.md) |
| `journey.device_enclose.alt_refused` 2 | `recog.rejected` | no chrome; ink unchanged | BR-B03 | [SRS-EP-10](./srs-logic.md) |
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
| A fifth *exclusive tool* for undo | Reject | [ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md) — Undo/Redo are actions after a gap |
| Confirmation dialog before creating a box | Reject | BR-B09 — best-effort + undo, no modals on e-ink |

---

## Superseded

Replaces the desktop-side journeys in
[infini vector-document srs-experience](../../../infini/features/vector-document/srs-experience.md)
for the ink-box paths, which deprecate with [SRS-IN-10] / [SRS-IN-11] / [SRS-IN-14].
