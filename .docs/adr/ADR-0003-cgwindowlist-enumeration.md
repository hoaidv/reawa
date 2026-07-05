---
id: ADR-0003
title: CGWindowList for window enumeration
status: accepted
date: 2026-07-05
deciders: [architect]
supersedes: null
---

# ADR-0003 — CGWindowList for window enumeration

## Context

Window picking and Absolute-mode lifecycle require reliable z-order, multi-display bounds, and on-screen area for the snapped target. Per-app `NSRunningApplication` enumeration was unreliable for stacking and global bounds during the Python era and early Swift port.

Constrains: [SRS-RW-20](modules/reawa/features/pen-input-absolute/srs-logic.md), [SRS-RW-21](modules/reawa/features/pen-input-absolute/srs-logic.md).

## Decision

Use `CGWindowListCopyWindowInfo` for picker listing and on-screen bounds. Filter: `kCGWindowLayer == 0`, exclude own PID, minimum size 40×40. Resolve picked window to AX element by PID + frame proximity for move/resize/follow.

**Close detection does not use CGWindowList membership** — see [ADR-0004](ADR-0004-stage-manager-lifecycle.md).

## Consequences

- Correct front-to-back picker ordering across displays.
- Stage Manager staged windows remain in OnScreenOnly list with shrunk CG bounds (enables minimize detection via area ratio).
- Picker uses **one overlay window per NSScreen** so every display receives mouse events (bug fix #1).

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| NSRunningApplication per-app windows | Unreliable z-order and bounds |
| AX-only enumeration | Incomplete list for picker; slower |
| Single full-desktop picker window | Click events only on display holding majority of window area |
