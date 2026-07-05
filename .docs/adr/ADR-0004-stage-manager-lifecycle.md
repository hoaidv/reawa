---
id: ADR-0004
title: Stage Manager lifecycle via CG/AX area ratio
status: accepted
date: 2026-07-05
deciders: [architect]
supersedes: null
---

# ADR-0004 — Stage Manager lifecycle via CG/AX area ratio

## Context

Under macOS Stage Manager, a staged window keeps `kAXMinimizedAttribute == false`, remains in `CGWindowListOptionOnScreenOnly`, and retains full-size AX frame — while CGWindow on-screen bounds shrink to a strip thumbnail. Prior heuristics (strip-thumbnail delta + frontmost app, OnScreenOnly absence, CGWindowList All absence) caused false overlay hide/show or false Relative revert.

Full investigation: [macOS window lifecycle investigation](../memory/macos-window-lifecycle-investigation.md).

Constrains: [SRS-RW-23](modules/reawa/features/pen-input-absolute/srs-logic.md#stage-manager-detection), [REQ-04].

## Decision

**Minimize (including Stage Manager staging)** is detected when the target window's live CG on-screen area divided by AX frame area is below **`stageAreaRatio = 0.5`**. This is target-specific and unaffected by sibling windows or focus changes.

**Close** is detected only via AX: `kAXRoleAttribute` returns `kAXErrorInvalidUIElement`. Never treat CGWindowList disappearance as close.

Disjunction for minimize (first match after close check):

1. `kAXMinimizedAttribute` — classic minimize / Cmd+M
2. Absent from `OnScreenOnly` — Cmd+H, off-stage Space
3. CG/AX area ratio below threshold — Stage Manager strip

## Consequences

- Overlay hides on Stage Manager minimize; Absolute mode stays active; restore re-shows overlay.
- R1/R2 regression scenarios (other window minimized first; focus sibling in same group) pass.
- Follow timer polls every 0.4 s via `WindowSnapController.snappedLifecycleState()`.
- Mid-restore animation may briefly sit between 0.5 and 1.0 area ratio (possible flicker — tune if needed).

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| `kAXMinimizedAttribute` alone | Stays false under Stage Manager |
| OnScreenOnly absence | Staged windows remain listed |
| CGWindowList All absence | Also true when staged; false close → Relative revert (bug #9) |
| Strip delta + frontmost | Global to display, not target-specific (bugs #10–#12) |
| `kCGWindowIsOnscreen` flag | Frequently absent under Stage Manager |

## Bug fix history (preserved)

| # | Symptom | Fix |
|---|---|---|
| 9 | Stage Manager minimize reverted to RELATIVE | Close only via AX invalidity |
| 10–12 | Overlay stuck or false hide with strip delta | CG/AX area ratio |
| 13 | Full-screen detection crash | Raw `"AXFullScreen"` attribute string |
| 14 | Stage Manager thumbnail pick snapped tiny bounds | `restoreWindow()` on pick before snap |

See also [pen-input-absolute srs-logic bug-fix-history](../modules/reawa/features/pen-input-absolute/srs-logic.md#bug-fix-history).
