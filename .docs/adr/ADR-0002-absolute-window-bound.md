---
id: ADR-0002
title: Absolute mode always window-bound
status: accepted
date: 2026-07-05
deciders: [architect, pm]
supersedes: null
---

# ADR-0002 — Absolute mode always window-bound

## Context

Absolute mode maps the reMarkable digitizer onto the Mac desktop. A free-floating absolute region would leave pen input in ambiguous states when the target app is unclear, and would complicate multi-display layouts. Product principle from [REQ-04]: pen input in Absolute mode must be clamped to a real application window.

## Decision

There is **no standalone absolute region**. Entering Absolute mode always runs the window picker. Cancelling (Esc, switching to Relative, or failed pick) reverts to Relative. Once snapped, pen mapping is clamped to the window; trackpad and mouse remain free for reaching other UI.

## Consequences

- `AbsoluteConfig` always includes snap metadata when mode is Absolute and active.
- Settings UI shows **Snapped window** context when bound.
- Closing the snapped window forces revert to Relative ([REQ-04] acceptance).
- Window picker must work on all connected displays ([ADR-0003](ADR-0003-cgwindowlist-enumeration.md)).

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| Free-floating draggable region | Pen could drive cursor outside intended app; poor UX for creative workflows |
| Snap optional / lazy | Stale region coords caused wrong mapping (bug #6 in fix history) |
| Full-screen only | Too restrictive for window-targeted design tools |
