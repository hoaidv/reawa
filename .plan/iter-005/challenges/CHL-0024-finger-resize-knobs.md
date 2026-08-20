---
id: CHL-0024
author: pm
target: [REQ-10, SRS-EP-21, SRS-EP-22, SRS-EP-23, SRS-EP-25]
severity: high
status: resolved
resolution: adopted
opened: 2026-08-20
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0024 — Finger may drive resize knobs

## Context

Human 2026-08-20, after visual review of [UI-EP-06](../design/hand-touch/):

> Finger can be used to manipulate resize knobs.

This reverses [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) “No subtle manipulation” (finger does **not** drive the 6 square anchors; resize stays pen) and the matching rows in [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) / [SRS-EP-22](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui).

The **size rule** stays: finger may hit a control only if its hit target is ≥ the primary ToolChip tile. Knobs become finger-eligible by making that hit ≥ the floor. Visual stays the hollow square from [CHL-0023](./CHL-0023-epaper-physical-scale.md) (not a 1 cm filled tile).

Human also: **do not** demonstrate `hand.pen_resize_after_finger_select` (mixed pen-after-finger-select is no longer a distinct grammar).

## Proposal

| Topic | Adopt |
|---|---|
| Pointer | Finger **and** pen drive SmartGroup **resize knobs**. Same live-direct resize as [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) / [REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation). Knob hit wins over box-move. |
| Hit | Knob **hit** ≥ primary ToolChip tile (CHL-0019 64 du; design package paints **10 mm** under CHL-0023). Visual **4 mm** hollow square. |
| Size rule | Unchanged for other sub-floor controls (rotation, connector end-kind, etc. stay pen until their hit meets the floor). |
| Journeys | Add `hand.finger_resizing`. Drop required `hand.finger_anchor_noop` and `hand.pen_resize_after_finger_select`. Retire `ind.finger_anchor_noop`. |

## Resolution

**Adopted** 2026-08-20 (pm). Human override. Not an interrupt — same TRACK-005 / REQ-10 slice; SM replans EP-038 AC + QA scenarios.

## Product doc updates

- `.docs/modules/epaper/prd.md` — [REQ-10](#hand-touch)
- `.docs/modules/epaper/features/ink-box/srs-logic.md` — SRS-EP-21
- `.docs/modules/epaper/features/ink-box/srs-ui.md` — SRS-EP-22
- `.docs/modules/epaper/features/ink-box/srs-quality.md` — SRS-EP-25
- `.docs/modules/epaper/features/tool-modes/srs-logic.md` — SRS-EP-23

## Interrupt / expedite (when applicable)

None.
