---
from: designer
to: sm
date: 2026-08-19
iter: iter-005
---

# Hand-off: Designer → SM — EP-037 hand-touch

## Context

Lane A design story [STORY-EP-037](../stories/STORY-EP-037.md) (Design hand-touch: one-finger pick/move and two-finger pan/zoom) is **done**. Package: `.plan/iter-005/design/hand-touch/` — [UI-EP-06](../design/hand-touch/ui-spec.md).

Painted the **new** REQ-10 grammar ([CHL-0022](../challenges/CHL-0022-shipped-no-device-pan.md)): one-finger pick/move + two-finger pan/pinch. Did **not** revive shipped “finger ignored.” One-finger empty is a **no-op** (not pan, not lasso) — not a global finger-ignore rule.

Composed ToolChip from [UI-EP-04](../../iter-004/design/toolchip-recognizers/) and overlay from [UI-EP-02](../../iter-003/design/device-selection-chrome/). No hand-tool tile. No finger-resize handles.

Dependents [STORY-EP-038](../stories/STORY-EP-038.md) and [STORY-EP-039](../stories/STORY-EP-039.md) have `ui_spec` / `scenes` / `hifi` copied. They stay **draft** until SM/architect/QA bind.

Did **not** edit `.docs/design/index.md` or `.docs/DESIGN.md` (SM stitch after join). Did **not** edit `srs-ui` / PRD.

## Asks

1. Stitch design index row for UI-EP-06 → this package after join with IN-034.
2. Route `/qa` BDD for EP-038 / EP-039 only after design+BDD gate (lock: do not `/dev` until design done + BDD).
3. Log experience gap for PM: `ink-box/srs-experience.md` has **no REQ-10 journeys**. Campaign override: did not hard-stop; scene inventory = SRS-EP-22 states + REQ-10 listed journeys. PM may thicken later — designer did not silently edit `.docs/modules/**`.

## Constraints

- Platform `data-platform="epaper"` (SRS-EP-22). Mechanical `adlc gate` Design-platform row still allowlists `ios|android|web|desktop` — owned by adopted [CHL-0002](../../iter-003/challenges/CHL-0002-epaper-platform-gate.md). Spec/SRS authoritative.
- Navigator preview **80%** of 1872×1404 (`data-preview-scale="mobile"`).
- Finger ≥64 du; handles 28 visual / 56 hit **pen-only**.
- `ind.two_finger_pan`: no extra product chrome (world translate + status).
- Unique system assets only under `design/system/assets/icon-hand-touch-*.svg` (+ copies of existing epaper icons for relative `../system/assets/` links).

## Out of scope

- STORY-IN-034 / `design/pen-button-map/`
- REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI
- Implement / BDD
- Promoting system comps to `.docs/design/system/` (iter close)

## Scene files

| State | File |
|---|---|
| `hand.finger_hit_box` (primary) | `hand-touch-finger-hit-box.html` |
| `hand.finger_moving` | `hand-touch-finger-moving.html` |
| `hand.finger_anchor_noop` | `hand-touch-finger-anchor-noop.html` |
| `hand.one_finger_empty` | `hand-touch-one-finger-empty.html` |
| `hand.two_finger_pan` | `hand-touch-two-finger-pan.html` |
| `hand.pinch` | `hand-touch-pinch.html` |
| `hand.pan_vs_move` | `hand-touch-pan-vs-move.html` |
| `hand.link_down_local_view` | `hand-touch-link-down-local-view.html` |
| `hand.pen_resize_after_finger_select` | `hand-touch-pen-resize-after-finger-select.html` |
| states showcase | `hand-touch-states.html` |
| navigator | `index.html` |

## Gate

- **ui-spec-gate:** pass (closed SRS-EP-22 inventory; 9/9 scenes; states showcase; iframe nav; 1-bit tokens; chip 64 du hittable). Experience 0a thin → **Concern** (not CHL), per campaign override.
- **adlc gate Design platform:** expected FAIL on `data-platform=epaper` — [CHL-0002](../../iter-003/challenges/CHL-0002-epaper-platform-gate.md).
- Other design CSS / interactivity / nav rows: expect PASS.

## Next

`/sm` join with IN-034; then `/qa` for EP-038 / EP-039 BDD. Not `/dev` yet.
