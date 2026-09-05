---
feature: clipboard
parent_req: [REQ-12]
version: 0.2.0
lifecycle: active
owner: pm
---

# SRS — In-document clipboard (Product)

PM-owned consolidation of [REQ-12](../../prd.md#clipboard). Module PRD stays lean; this file is
the product contract for Architect / QA / Dev. Chrome ids live in
[SRS-EP-32](../ink-box/srs-ui.md#srs-ep-32-clipboard-ui); clone/parent algebra in
[SRS-EP-31](../device-document/srs-logic.md#srs-ep-31-clipboard).

**Needs design: no.** Chrome is frozen here and in SRS-EP-32. No design story.

Adopted [CHL-0031](../../../../../.plan/iter-005/challenges/CHL-0031-clipboard-tap-paste.md)
(2026-09-04): paste is on the **normal** context toolbar from a **tap location**, not long-press.

## Intent / JTBD

A creator has already drawn a cluster that is *right*. They need to **duplicate it or move a copy**
on the tablet — without redrawing, without the laptop, and without the macOS pasteboard. Copy and
cut hang on the selection they already have. Paste is placed **where they last tapped** (empty
canvas or a node), because the device has no right-click.

## In scope / out of scope (feature grain)

| In scope | Out of scope |
|---|---|
| One process-global in-document slot | OS / Qt / macOS pasteboard; multi-slot; cross-app paste |
| Copy / cut of the current selection in **SelectionMode** | Copy/cut/paste from InkMode or EraserMode |
| Paste from the **Selected / tap** context toolbar when a tap location exists | Long-press / hold toolbar (retired); ToolChip paste; barrel; drag-to-place |
| Paste origin = tap world point (union AABB top-left) | Fixed +24 world offset (retired with ADR-0024) |
| Local undo of cut and paste | Publishing cut/paste on the wire this track ([REQ-07](../../prd.md#one-way-sync) postponed) |
| Tap vs travel (≤1 mm panel = tap-select) so stylus can set a paste location | A new exclusive Mode; 500 ms hold menu |

## [SRS-EP-73] Clipboard product rules {#srs-ep-73-clipboard-product}

<!-- lifecycle: active -->

**Parent:** [REQ-12](../../prd.md#clipboard).

### Business rules / eligibility / policy

| Rule id | Statement (product language) | Notes |
|---|---|---|
| BR-C01 | **One slot, process-global.** A later copy/cut replaces it. The slot is not a document node, not in `doc_load` / SVG, and not on `DeviceDocument`. App restart empties it because the process dies; `doc_load` does **not** clear it. | Singleton |
| BR-C02 | **Copy does not change the document.** Slot ← clone; **0** undo; selection unchanged. | |
| BR-C03 | **Cut = copy then delete.** Slot ← clone; selected roots removed; **one** undo restores them. Empty groups/SmartGroups are **left in the tree**. | Diverges from erase empty-group cleanup |
| BR-C04 | **Paste is one undoable gesture.** New ids; union AABB **top-left** lands on the **tap** world point; items keep relative layout. One undo removes the copies. Slot unchanged. Empty slot → 0 nodes, 0 undo, **0 banners**. | |
| BR-C05 | **Copy/cut chrome** only when SelectionMode (`sel_rect` or `sel_freeform`) and `SelectionPhase::Selected` with a non-empty selection. Tiles live on the **normal** context toolbar (selection AABB). | `cta.copy` / `cta.cut` |
| BR-C06 | **Paste chrome** when the slot is non-empty **and** a tap location exists (tap empty canvas, or tap a node) **and** the selection is settled (`Selected`, or Idle empty-tap). Same toolbar as copy/cut when a node is selected; paste-only strip at the tap point when the canvas is empty. **Not** after freeform / marquee (no tap location). **Not** while a move / resize / lasso / marquee is in flight. Not on the ToolChip. | `cta.paste` |
| BR-C07 | **Tap vs travel.** Panel travel ≤ **1 mm** then lift is a tap. Node tap: select and record paste origin. Empty tap while selected: **deselect only** (no paste origin, no chrome). Empty tap while already idle: record paste origin. Travel > 1 mm begins Move / lasso / marquee as today. **No** 500 ms hold menu. | Stylus tap equals finger tap |
| BR-C08 | **Tap empty while idle** with a non-empty slot: paste origin = tap; paste-only chrome at the tap panel point (clamped). Empty slot → **0** paste chrome. **Tap empty while selected:** clear selection, **0** paste origin, **0** chrome (CHL-0007). | |
| BR-C09 | Toolbar clamp (empty-canvas paste strip) does **not** move the paste origin. | Chrome ≠ geometry |
| BR-C10 | Paste origin **clears** on: empty tap while a selection exists, freeform / marquee that is a real gesture (not a tap), pan, mode switch, camera pan/zoom, successful paste. A new tap replaces it. | |
| BR-C11 | **Stylus tap** in SelectionMode (travel ≤ 1 mm) **selects** like finger. | |
| BR-C12 | **Clone grain.** Slot roots are selected nodes that are **not** descendants of another selected node. If a root is selected and **no descendant is selected**, clone the **full subtree** (tap-select of an ink-box copies the box and its ink). If some descendants are also selected, keep only the selected-descendant spine (unselected siblings dropped). Children of a slot root are not extra paste roots. | |
| BR-C13 | **Paste parent.** After translate, hit-test at the tap (same as tap-select). If the hit is a legal parent (SmartGroup, Frame, Group), that is the parent (tap on an ink-box **puts the copy in the box**). Else walk ancestors with **20% overlap** vs natural boundary. Else **document root**. Do not 20%-test the document root. Free ink parented into a SmartGroup uses the same local-ink rule as draw-into membership. | |
| BR-C14 | **Paste onto live originals is refused.** If the tap hit **or** the assigned parent is in the **live copied source subtree**, show an error on the toolbar, **0** document change, slot kept. Copy-only (after cut the sources are gone). | Enclose-style refuse string |
| BR-C15 | **Connectors.** Copy only what is selected. Endpoint ids inside the slot remap; endpoints outside stay on the live originals. | |
| BR-C16 | **No wire this track.** Cut and paste commit locally and push the undo ring; they **do not** enqueue `doc_change`. Infini apply is a later track. | Does not steal REQ-07 |
| BR-C17 | **Opaque kinds** that round-trip may be copied. Paste inserts cloned bodies as-is. | |

### Edge cases

| Case | Expected product behavior |
|---|---|
| Empty selection copy/cut | No-op; slot unchanged; 0 undo |
| Empty slot paste / tap empty | 0 nodes; 0 undo; **no** paste chrome |
| Copy then tap empty | First tap deselects only (0 paste chrome). A second tap while idle sets paste origin; copies become document-root if the tap is not a legal parent |
| Copy then tap an ink-box (not a live source) | Paste parents into that SmartGroup |
| Copy then tap a live source node | Error; slot kept; originals unchanged |
| Cut then tap empty and paste | Sources gone → BR-C14 does not fire |
| Tap-select an ink-box then copy | Slot holds the box **and its children** |
| Parent + descendant both selected | One slot root (the parent) containing only the selected descendants |
| One paste, many parents | Allowed; still **one** undo entry |
| Cut last child of a SmartGroup | Child gone; **empty group remains** |
| Freeform / marquee selection | Copy/cut if non-empty; **no** paste button (no tap location) |
| Finger-down then drag (move / resize) with a tap location | Paste chrome **hidden** for the gesture; tap location kept; chrome may return when `Selected` or Idle again |
| `doc_load` / new epoch | Slot **kept** (ids reminted at paste) |
| Link up or down | Same local result; **0** publish |

### Acceptance (drives BDD / stories)

- Given a non-empty selection in SelectionMode, When the creator copies, Then the document is unchanged, the slot is non-empty, and undo depth is unchanged.
- Given a tap-selected ink-box, When the creator copies, Then the slot holds that SmartGroup and its children.
- Given a non-empty selection in SelectionMode, When the creator cuts, Then the selected roots are gone, the slot is non-empty, and one undo restores the roots (empty parent groups still present if they were empty after the cut).
- Given a non-empty slot, When the creator taps empty or a node and taps Paste, Then new ids exist, the paste union AABB top-left equals the **tap** world point (±1 px @ 100% zoom), relatives are preserved, and the slot is unchanged.
- Given free ink in the slot, When the creator taps an ink-box that is not a live source and pastes, Then the copy is a child of that SmartGroup.
- Given a non-empty slot after copy, When the creator taps a live source node and taps Paste, Then 0 nodes change, an error is shown, and the slot is unchanged.
- Given an empty slot, When the creator taps empty, Then selection clears and **0** paste chrome appears.
- Given a non-empty selection, When the creator taps empty canvas, Then selection clears and **0** paste chrome appears (a further idle tap may set paste origin).
- Given an empty slot, When Paste is invoked, Then 0 nodes change and 0 undo entries are pushed.
- Given cut then paste, When the creator undoes once, Then the copies are gone and the originals are still gone; a second undo restores the originals.
- Given pointer-down on a node in SelectionMode with travel ≤ 1 mm and lift, When observed, Then the node is selected and its world pose is unchanged (0 nudge).

### Implemented via

| Concern | Pointer |
|---|---|
| Logic | [SRS-EP-31](../device-document/srs-logic.md#srs-ep-31-clipboard) · tap vs travel in [SRS-EP-11](../ink-box/srs-logic.md#srs-ep-11-hold-still) |
| UI | [SRS-EP-32](../ink-box/srs-ui.md#srs-ep-32-clipboard-ui) |
| Quality | [SRS-EP-33](../device-document/srs-quality.md#srs-ep-33-clipboard-quality) |

---

## Superseded

The +24 world-unit paste offset and “slot on DeviceDocument / publish `duplicate_subtree` this track”
rows of [ADR-0024](../../../../adr/ADR-0024-in-document-clipboard.md) (proposed). Replacement:
[ADR-0037](../../../../adr/ADR-0037-device-clipboard-singleton.md).

Long-press / hold-toolbar paste (BR-C06–C10 as of 2026-09-04 morning) — replaced the same day by
tap-origin paste ([CHL-0031](../../../../../.plan/iter-005/challenges/CHL-0031-clipboard-tap-paste.md)).
