---
from: qa
to: sm
date: 2026-08-20
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Quality Assurance Engineer → Scrum Master — STORY-EP-038 verified

[STORY-EP-038](../stories/STORY-EP-038.md) (One-finger hit box: select freeform and move) is **done**. No defect. Do not start [STORY-EP-039](../stories/STORY-EP-039.md) until SM dispatches.

## Result

**PASS.** All 7 Gherkin scenarios in [hand-touch-one-finger.feature](../../../.docs/modules/epaper/features/ink-box/bdd/hand-touch-one-finger.feature) map to `epaper/tests/hand_touch_test.cpp`. Host suite `epaper/tests/run_device_document_test.sh` exit 0, including `hand_touch_test ok` and `manipulate_test ok`.

## Scenario mapping

| Feature scenario | Host test |
|---|---|
| Finger-down inside a box arms sel_freeform and selects | `test_finger_down_box_select_freeform` |
| Finger drag inside the selected box moves live-direct with 0 pan | `test_finger_drag_box_zero_pan` |
| Finger on a resize knob resizes live-direct with 0 pan | `test_finger_knob_resize_zero_pan` |
| One-finger empty travel at or below 10 mm is palm-rest | `test_empty_palm_rest` |
| One-finger empty travel past 10 mm pans locally with Infini unchanged | `test_empty_pan_local_infini_unchanged` |
| ToolChip 64 du tap still holds REQ-03 | `test_chip_wins_over_empty_pan` |
| One-finger empty pan while following Infini turns Epaper follow off | `test_follower_local_nav_turns_follow_off_then_pans` |

Extra host probes (not extra scenarios): publish-only-if-Infini-follow, hit priority chip>knob>box, follow parse.

## UI spot-check ([UI-EP-06](../design/hand-touch/ui-spec.md))

Regions/components vs `hand_touch.hpp` + `TabletAppFilter` + `TabletCanvasItem`. **No Spec drift DEF.**

- **ToolChip** — chip hit wins over box/empty; 64 du primary tile; `sel_freeform` arms exclusive tool; recognizer toggles dim via `recogDimmed()` and stay armed; chip still hittable (Chip > Box).
- **ovl.selection_bounds** — box down reuses live-direct `beginSelectionGesture`; overlay is existing UI-EP-02 chrome.
- **Knobs** — `kFingerHandleHitDu = 64` (≥ primary tile per [CHL-0024](../challenges/CHL-0024-finger-resize-knobs.md)); knob wins over box-move; 0 pan.
- **Palm vs pan** — 10 mm / 89 du @ 226 dpi; ≤ threshold = no pan / tool unchanged; > threshold = local `drawingRegion` pan; follower sets `follow.direction = none` **then** pans; viewport up only if `epaper_to_infini`.

Did not edit design HTML.

## Defects

None.

## Residual risks (not DEF)

- Device/Qt `epaper_bin` not built (`cmake`/`qmake` not on PATH). No RM2 panel run.
- Chip p95 ≤300 ms and live-direct ≥5 Hz / 0 px jump **not re-measured on panel**. Host tests cover classifier + follow/pan gates; timing is the existing live-direct path (`kSelectionGhostMinIntervalMs = 200` → 5 Hz analog). Limitation only, per verify brief.

## Asks

1. `/sm` — record EP-038 verified; dispatch `/dev` for EP-039 when ready.
2. `/pm` may gate-close this story. Do **not** start EP-039 from this lane.

## Constraints

- Did not edit `src/`, PRD, SRS, MASTER, execution board, or design HTML.
- Did not `adlc rollup`. Did not git commit.
- Two-finger pan/pinch (EP-039), Infini apply (IN-033), follow-toggle chrome, last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) remain out of scope.
