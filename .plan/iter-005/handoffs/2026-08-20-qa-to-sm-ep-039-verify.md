---
from: qa
to: sm
date: 2026-08-20
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Quality Assurance Engineer → Scrum Master — STORY-EP-039 verified

[STORY-EP-039](../stories/STORY-EP-039.md) (Two-finger pan and pinch; publish viewport) is **done**. No defect. [STORY-EP-038](../stories/STORY-EP-038.md) stays **done** (one-finger host tests still pass). Infini apply remains [STORY-IN-033](../stories/STORY-IN-033.md) — not this sign-off.

## Result

**PASS.** All 8 Gherkin scenarios in [hand-touch-two-finger.feature](../../../.docs/modules/epaper/features/region-sync/bdd/hand-touch-two-finger.feature) map to `epaper/tests/hand_touch_test.cpp`. Host suite `epaper/tests/run_device_document_test.sh` exit 0, including `hand_touch_test ok` (EP-038 + EP-039) and `manipulate_test ok`.

## Scenario mapping

| Feature scenario | Host test |
|---|---|
| Two-finger pan for 5 s applies the local map before the next pen sample | `test_two_finger_pan_map_before_pen` |
| Two-finger pinch scales uniformly with the same map-apply bar | `test_two_finger_pinch_uniform_scale` |
| Infini follow off leaves Infini's camera unchanged | `test_two_finger_follow_none_zero_viewport_up` |
| Infini follow on publishes the local viewport from the tablet | `test_two_finger_follow_on_publishes_viewport_up` |
| Second finger does not start pan while box-move is in flight | `test_second_finger_blocked_during_box_move` |
| Second finger does not start pan while resize is in flight | `test_second_finger_blocked_during_resize` |
| Link down still changes the local viewport | `test_two_finger_link_down_local_view` |
| Two-finger pan while following Infini turns Epaper follow off | `test_two_finger_follower_local_nav_turns_follow_off` |

EP-038 regression (same binary): all 7 one-finger scenarios still present and passing.

## UI spot-check ([UI-EP-06](../design/hand-touch/ui-spec.md))

Regions/components vs `hand_touch.hpp` + `TabletAppFilter` + `TabletCanvasItem`. **No Spec drift DEF.** Did not require Infini canvas match ([STORY-IN-033](../stories/STORY-IN-033.md)).

- **`hand.two_finger_pan`** — `applyTwoFingerPanPinch` updates local `drawingRegion`; no extra chrome; default `follow=none` emits 0 viewport up (Infini match not implied).
- **`hand.pinch`** — uniform scale (`scale_x == scale_y`); 0 rotation / skew; AABB stays axis-aligned.
- **`hand.pan_vs_move`** — `canPromoteToTwoFinger` is false during box-move/resize; `beginTwoFingerTouch` returns false; in-flight move/resize continues; world unshifted.
- **`hand.link_down_local_view`** — local map still pans; 0 viewport up; host `publishQueued` flag only (no new ToolChip/USB chrome).
- **Publish** — viewport up only if `follow.direction = epaper_to_infini`; `source: epaper`; `settle: true` on two-finger end. Inbound Infini viewport applies only while `infini_to_epaper`. Follower local-nav sets `none` then pans.

Did not edit design HTML.

## Defects

None.

## Residual risks (not DEF)

- Device/Qt `epaper_bin` not built (`cmake`/`qmake` not on PATH). No RM2 panel run.
- p95 map-apply ≤100 ms is the host `applyTwoFingerPanPinch` clock, not re-measured on device. 5 s pan is not wall-clocked; one step + budget covers the scenario.
- Link-down ToolChip queued hatch / USB-unplugged badge is host `publishQueued` only — Qt does not invent that chrome.
- Infini canvas match after settle is [STORY-IN-033](../stories/STORY-IN-033.md), not this story.

## Asks

1. `/sm` — record EP-039 verified; keep EP-038 done.
2. `/pm` may gate-close this story. Do **not** start Infini apply (IN-033) from this lane unless SM dispatches.

## Constraints

- Did not edit `src/`, PRD, SRS, MASTER, execution board, or design HTML.
- Did not `adlc rollup`. Did not git commit.
- Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md), follow-toggle chrome (EP-049/EP-053), Device Settings / barrel, `macOS/` remain out of scope.
