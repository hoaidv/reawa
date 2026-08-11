---
story: STORY-EP-004
srs: SRS-EP-04
date: 2026-08-11
status: complete
---

# Spike — RM2 capacitive touch reachability

## Method

1. **Code SoT (pre-spike):** `TabletAppFilter` handled `QTabletEvent` + `QMouseEvent` only —
   no `QTouchEvent` cases. Pen ink does not need touch.
2. **Harness:** Added a non-consuming probe in `epaper/tabletappfilter.cpp` for
   `QEvent::TouchBegin|Update|End|Cancel`. On first touch it logs:
   `[STORY-EP-004] capacitive touch REACHABLE via QEvent::TouchBegin (filter path: TabletAppFilter::eventFilter)`.
3. **QPA hints already present** in `epaper/main.cpp` (`QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS`,
   `evdevtablet`) — the stack is *prepared* for a touchscreen device; the app simply never
   listened before.

## Result (dev / CI without RM2 hardware)

| Field | Value |
|---|---|
| Reachable in this environment | **unknown / not observed** (no physical RM2 touch in this session) |
| API path if/when events arrive | `QEvent::Touch*` → `TabletAppFilter::eventFilter` |
| Pen path | Unchanged — still primary ink path |

**On-device follow-up:** run the epaper binary on RM2, tap the glass with a finger (not pen),
confirm the `[STORY-EP-004]` log line. If it appears → **yes**. If never → **no**.

## Fallback recommendation (for STORY-EP-003 design)

Until on-device confirmation says **yes**, design must not assume a finger-only toolbar:

1. **Preferred if touch yes:** finger taps on a bottom/side tool strip (`Selection · Pen · Ink-box`).
2. **Fallback if touch no:** **pen-on-strip** — tool affordances are hit-targets for the *pen*
   (not finger), OR a **hardware button** cycle if product later exposes one.
3. Do **not** invent a second product; Spec for EP-003 should show the primary strip and note
   the fallback variant in one package.

## Unblocks

STORY-EP-003 may leave `draft` once designer reads this spike note (design for strip + fallback).
