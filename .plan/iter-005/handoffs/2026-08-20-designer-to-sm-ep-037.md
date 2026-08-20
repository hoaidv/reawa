---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, qa]
---

# Hand-off: Designer — UI-EP-06 follows CHL-0024

## Story

[STORY-EP-037](../stories/STORY-EP-037.md) remains **done**. Package revised in place.

## Package

`.plan/iter-005/design/hand-touch/`

- Added `hand-touch-finger-resizing.html` (`hand.finger_resizing`)
- Removed `hand-touch-finger-anchor-noop.html` and `hand-touch-pen-resize-after-finger-select.html`
- Knob hit **10 mm**, visual **4 mm**; finger and pen
- Navigator + ui-spec + states showcase updated

## CHL

Painted [CHL-0024](../challenges/CHL-0024-finger-resize-knobs.md) (pm adopted). [CHL-0023](../challenges/CHL-0023-epaper-physical-scale.md) still open (physical mm vs 64 du).

## Next

`/sm` then `/qa` EP-038.
