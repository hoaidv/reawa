---
feature: tool-modes
parent_req: [REQ-03]
version: 0.2.0
lifecycle: active
---

# SRS — Tool modes Epaper (Quality)

## [SRS-EP-06] Tool responsiveness and ink-latency protection

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Round-trip budgets replaced by local budgets;
     ghost-correction scenario retired. Same id, content revised. -->

> **Revised 2026-08-13.** The enclose budget used to be measured "after the Infini op". There is no
> Infini op. Every budget here is now local, and two scenarios are added that the pilot could not
> have: **offline parity** and **commit fidelity**.

### Quality-attribute scenarios

| Scenario | Stimulus | Response measure |
|---|---|---|
| Tool switch latency | Finger taps a tool | Active indicator updated **p95 ≤300 ms**; partial refresh only |
| **Ink latency non-regression** | Draw with `pen` after the document, recognizer, and undo ring ship | p95 ≤30 ms pen-down → pixel — **equal to the pre-toolbar baseline within measurement error** ([SRS-EP-01](../local-pen-ink/srs-logic.md)) |
| Tool-independent ink path | Draw the same stroke in `pen` and in `ink_box` | Identical local raster and identical wire payloads while the pen is down; the tools differ only at pen-up |
| Chrome exclusion | Pen stroke started on the ToolChip bounds | **0** ink pixels drawn there; 0 strokes emitted |
| Refresh isolation | Switch tools repeatedly | 0 full-panel refreshes attributable to tool switching |
| Ghosted state legibility | Switch tools while a full refresh trails | Active tool identifiable from shape/fill alone on the un-settled frame |
| **Enclose, local** | Draw an armed enclose over ink | Smart Group visible on panel **p95 ≤500 ms after pen-up**, with **0 messages required from the peer** |
| **Selection pick feel** | Pen down on a box | Selection affordance **≤100 ms**, hit-tested against the local document |
| **Commit fidelity** | Complete 20 scripted move/resize gestures | Committed geometry = last previewed geometry; **0 px jump, 0 snap-backs** |
| **Offline parity** | Run the scripted 10-gesture create+manipulate set with the link down | 100% identical results vs the linked run; 0 tools unavailable |
| **Publish latency** | Commit an op with the link up | Mirror updated **p95 ≤300 ms** ([SRS-IN-08](../../../infini/features/tablet-sync/srs-quality.md)) |
| Touch fallback | Touch layer unavailable | Device still inks; `pen` forced; the reason is visible, not silent |
| Session loss | Session drops mid-gesture | Gesture completes and commits locally; no crash, no lost ink; change queues |

Retired scenarios: *enclose round trip after the Infini op*, *ghost correction on next snapshot*.
Both described behaviour this rework deletes.

### Correctness ties

- Arming `ink_box` must not alter sampling, the Round 19 map, stroke width, or the local paint path
  while the pen is down. It changes only what happens at pen-up.
- The device **is** the writer: it fits the rectangle, tests containment, and mutates its tree
  ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §1). ADR-0013 §3 is reversed.
- No inbound message may alter the document except a handshake-gated `doc_load`
  ([SRS-EP-08](../device-document/srs-logic.md)).

### A11y / resilience

- ToolChip targets are **32×32** (pilot, CHL-0003) with pen-on-chip fallback when finger miss
  rate is unacceptable; do not re-expand to a full-band ≥120 px strip without a new challenge.
- Active state distinguishable without color **and** without motion — 1-bit panel.
- `pen` is always reachable in ≤1 tap from any state; the creator can never be trapped in a
  non-drawing tool.
- With no session, the device keeps **full** function (local ink **and** local editing) rather than
  degrading — only publishing waits, and that state is visible.

### Dual-ask table (state → Designer + QA)

| State id | Designer scene / Spec | QA AC / BDD |
|---|---|---|
| `tool.pen` | tool-modes package | REQ-03 default + ink latency |
| `recog.ink_box.on` / `.off` | tool-modes package | toggle armed; ink path identical while pen is down |
| `recog.connector.on` / `.off` | tool-modes package | toggle independent of ink-box |
| `recog.rejected` | tool-modes package | stroke stays ink; no banner |
| `recog.dimmed` | tool-modes package | both toggles dimmed under Selection; armed state kept |
| `tool.selection.idle` | tool-modes package | pen inert on canvas |
| `tool.selection.selected` | ink-box package | real bounds + handles from the local document |
| `tool.selection.moving` | ink-box package | real ink follows pen; commit = release |
| `tool.selection.resizing` | ink-box package | per `inkScaleMode`; UV preserved |
| `session.pending_changes` | tool-modes package | tools usable; queue visible |
| `session.reloading` | tool-modes package | load applied only after queue drains |
| `manipulation.unavailable` | ink-box package | below LOD; 0 accidental transforms |
| `touch.unavailable` | tool-modes package | fallback to pen; reason visible |
| `orient.gutOnTop` | tool-modes package | chip on oriented top; exclusion rect follows |

---

## [SRS-EP-43] Barrel dispatch quality {#srs-ep-43-barrel-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-18](../../prd.md#pen-buttons) (dispatch + catalogues) · [REQ-20](../../prd.md#device-settings) (device persist). **Constrains:** [SRS-EP-41](./srs-logic.md#srs-ep-41-barrel-dispatch), [SRS-EP-52](./srs-ui.md#srs-ep-52-pen-map-editor), [SRS-EP-53](./srs-logic.md#srs-ep-53-pen-map-author). Does **not** steal [SRS-EP-06](#srs-ep-06) parent (REQ-03).

| Scenario | Target |
|---|---|
| 1-button default Click (no move) | Toggle `pen` ↔ `sel_freeform` p95 ≤**300 ms**; **0** hold-move |
| 1-button default Hold-move | Temporary **erase** until release; **0** click toggle on release |
| 2-button default B2 Hold-move | Temporary erase until release; B1 unchanged |
| Rebind Hold-move `drag_node_under_tip` on node | Live-direct bar (0 px / ≥5 Hz); empty canvas: **0** move, **0** lasso |
| 0-button pen | **0** button gestures; REQ-03 still works |
| 20-gesture mixed click/hold fixture | **0** events fire **both** click and hold-move |
| On-device rebind (session up or down) | Next gesture uses map p95 ≤**300 ms**; in-flight unchanged; **0** lost local binds; persist does **not** wait on Infini |
| Device restart (same Epaper) | Next gesture uses device-stored map p95 ≤**300 ms** after first HID report; Infini holds **0** copy |
| Infini connected rebind | **0** `doc_*`; **0** `pen_button_map`; **0** Infini app-settings maps |
| Other Epaper, factory defaults, same Infini document | **0** inherited barrel map |
| Settings 0-button | **0** slot rows; **0** fake bindings |
| Click catalogue (inline) | Only `toggle_pen_freeform` · `toggle_pen_eraser` · `off` (**0** Undo, **0** sheets) |
| Hold-move catalogue (inline) | Only `temp_erase` · `drag_node_under_tip` · `off` (**0** temp freeform/rect, **0** sheets) |
| Leading entry tile | Tap `cta.pen_map_open` → Settings · Pen buttons p95 ≤**300 ms**; exclusive tool unchanged |

Retired scenario: *Infini restore on later session* — Infini persist/restore is withdrawn ([ADR-0031](../../../../adr/ADR-0031-device-settings-persist-on-epaper.md)).

---

## [SRS-EP-48] Manual create quality {#srs-ep-48-manual-create-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-17](../../prd.md#manual-create) (Should). **Constrains:** [SRS-EP-44](./srs-logic.md#srs-ep-44-manual-create-routing), [SRS-EP-45](../device-document/srs-logic.md#srs-ep-45-manual-insert), [SRS-EP-46](../connector-ink/srs-logic.md#srs-ep-46-manual-connector).

| Scenario | Target |
|---|---|
| Place Frame | Node at placed bounds ±1 px @ 100% zoom; p95 ≤**300 ms**; one undo removes it |
| Manual connector between two bindable nodes | Same warp contract as REQ-09 |
| Manual attach | SRS-EP-40 bars hold |
| Primitive ellipse/rect/line | Parameterized geometry (not polyline stand-in); survives REQ-07 mirror |
