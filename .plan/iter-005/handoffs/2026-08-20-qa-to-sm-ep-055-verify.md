---
from: qa
to: sm
date: 2026-08-20
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Quality Assurance Engineer → Scrum Master — STORY-EP-055 verified

[STORY-EP-055](../stories/STORY-EP-055.md) (Epaper follow Infini — toggle, exclusion, disconnect) is **done**. No defect. Did not edit `infini/`. Did not start last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md). Infini follow toggle remains [STORY-IN-037](../stories/STORY-IN-037.md).

## Result

**PASS.** All 5 Gherkin scenarios in [viewport-follow-epaper.feature](../../../.docs/modules/epaper/features/region-sync/bdd/viewport-follow-epaper.feature) map to `epaper/tests/viewport_follow_test.cpp`. Host suite `epaper/tests/run_device_document_test.sh` exit 0, including `viewport_follow_test ok` (EP-055) and `hand_touch_test ok` (EP-038 + EP-039).

## Scenario mapping

| Feature scenario | Host test |
|---|---|
| Enabling Epaper follow from both-off applies Infini viewport | `test_enable_from_both_off_applies_infini_viewport` |
| Tapping Epaper follow while Infini is following takes over | `test_tap_while_infini_following_takes_over` |
| Connection lost while following forces follow off | `test_connection_lost_forces_follow_off` |
| Reconnect does not restore Epaper follow | `test_reconnect_does_not_restore` |
| Local pan while following Infini turns Epaper follow off | `test_local_pan_while_following_turns_off` |

Extra host probe (not an extra scenario): `test_hello_does_not_carry_follow`.

## UI spot-check ([UI-EP-07](../design/viewport-follow-epaper/ui-spec.md))

Regions/components vs `viewport_follow.hpp` + `Main.qml` FollowToggle + `TabletCanvasItem`. **No Spec drift DEF.**

- **FollowToggle sibling of ToolChip** — `Main.qml` `id: followToggle` is a trailing `Rectangle` at `z: 20`, sibling of `id: toolChip`, not nested inside the chip. Host `followToggleRect` vs `chipWidth()` asserts 0 containment / 0 intersection.
- **Not a fourth exclusive** — ToolChip Repeater still three tiles (`sel_rect`, `sel_freeform`, `pen`). `exclusiveToolCount() == 3`. `hitId(Pen) != btn.viewport_follow`. Enabling follow leaves exclusive tool `pen`.
- **Closed inventory** — one icon toggle `btn.viewport_follow`; invert when `followPressed` (`aria-pressed` analog); hatch + `MouseArea` disabled when `followUnavailable`; 10 mm / 64 du tile, trailing inset.
- **States** — host `uiStateId` covers `follow.following_infini`, `follow.peer_following_you`, `follow.connection_lost`, `follow.reconnect_stays_off`. Disconnect → `none` before next gesture; reconnect stays off until tap. 0 `doc_*` from the toggle.

Did not edit design HTML. Did not edit `infini/`.

## Defects

None.

## Residual risks (not DEF)

- Device/Qt `epaper_bin` not built (`cmake`/`qmake` not on PATH). No RM2 panel run. **Missing cmake is residual, not auto-fail.**
- `epaper/CMakeLists.txt` has no CMake test target for `viewport_follow_test` (host suite is the shell script). Icons `icon-epaper-viewport-follow.png` / `-inv.png` are in the Qt resource list.
- p95 map-apply ≤100 ms and p95 exclusivity ≤300 ms are host clocks on `tapToggle()`, not re-measured on device.
- QML uses `followPressed` / `followUnavailable` invert+hatch, not Qt `Accessible.pressed` / `Accessible.disabled` attributes. Host `ariaPressed()` / `ariaDisabled()` cover the BDD assertions.
- Infini follow toggle and Infini canvas match remain [STORY-IN-037](../stories/STORY-IN-037.md) / [STORY-IN-033](../stories/STORY-IN-033.md).

## Asks

1. `/sm` — record EP-055 verified; do not dispatch last-writer ADR-0023 from this lane.
2. `/pm` may gate-close this story. Infini follow (IN-037) is a sibling WIP, not this sign-off.

## Constraints

- Did not edit `infini/`, PRD, SRS, MASTER, execution board, or design HTML.
- Did not `adlc rollup`. Did not git commit.
- Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md), Device Settings / barrel, `macOS/`, REQ-15, REQ-08, CHL-0011, CHL-0012 remain out of scope.
