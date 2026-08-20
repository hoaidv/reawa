---
from: pm
to: architect
date: 2026-08-20
iter: iter-005
cc: [sm, designer, qa]
---

# Hand-off: PM — finger resize knobs (CHL-0024 adopted)

## Verdict

**READY-WITH-CONCERNS** for the REQ-10 one-finger slice. Product docs updated. Architect: confirm live-direct resize reuse of SRS-EP-11 (no new SRS id). SM: replan EP-038 AC + QA scenarios.

## Decision

Human 2026-08-20: **finger may drive resize knobs.** Adopted [CHL-0024](../challenges/CHL-0024-finger-resize-knobs.md).

- Knob **hit** ≥ primary ToolChip tile; visual stays the small hollow square.
- Same live-direct bar as pen resize (REQ-06 / SRS-EP-11). Knob wins over box-move.
- Size rule otherwise unchanged (rotation, end-kind, etc. stay pen until they meet the floor).
- Drop required journeys `hand.finger_anchor_noop` and `hand.pen_resize_after_finger_select`. Add `hand.finger_resizing`.

## Product docs

- [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch)
- [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger)
- [SRS-EP-22](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui)
- [SRS-EP-25](../../../.docs/modules/epaper/features/ink-box/srs-quality.md#srs-ep-25-one-finger-quality)
- [SRS-EP-23](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-23-finger-tool-switch)
- [SRS-EP-24](../../../.docs/modules/epaper/features/region-sync/srs-logic.md) — two-finger still blocked while move **or resize** in flight

## Concerns

- [CHL-0023](../challenges/CHL-0023-epaper-physical-scale.md) still **open** (10 mm tiles vs 64 du). Design package paints millimetres; product still cites CHL-0019 64 du as the floor.
- Glossary Finger-eligible row updated; architect owns further glossary polish.

## Next

`/architect` if EP-038 needs a dedicated resize SRS bind. Otherwise `/sm` then `/qa` for EP-038 scenarios.
