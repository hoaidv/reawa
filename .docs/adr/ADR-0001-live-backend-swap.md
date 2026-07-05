---
id: ADR-0001
title: Live backend swap without SSH reconnect
status: accepted
date: 2026-07-05
deciders: [architect, pm]
supersedes: null
---

# ADR-0001 — Live backend swap without SSH reconnect

## Context

Early implementations reconnected SSH after changing output mode (Relative ↔ Absolute) or completing a window snap. That caused races: overlay state reset while the new session was still connecting, leaving input paused and the picker re-triggered. Quality goal: mode switches must feel instant with no pen stream interruption beyond intentional pause during picking.

Constrains: [SRS-RW-16](modules/reawa/features/pen-input-relative/srs-logic.md), [SRS-RW-19](modules/reawa/features/pen-input-absolute/srs-logic.md), [REQ-03], [REQ-04].

## Decision

`DriverSession` runs on a dedicated background thread with one open SSH stream. When `outputMode` changes, the session calls `cleanup()` on the old backend and instantiates the new backend (`RelativePenDriver`, `AbsolutePenDriver`, or `NativeStylusBackend`) **in place** without tearing down SSH. Live `DeviceConfig` is read from an in-memory snapshot updated by `ConnectionManager`.

## Consequences

- Mode switches are fast and do not race with overlay state.
- `pause()` / `resume()` discard frames during window picking while SSH stays connected.
- Native Stylus failure can fall back to last mouse mode within the same session.
- Session thread must safely swap backends and handle cleanup on every mode change.

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| Reconnect SSH on mode change | Caused races, input stuck paused, picker re-entry loops |
| Separate SSH session per mode | Resource waste; same race on handoff |
| Process-per-mode | Overkill for a menu bar utility |
