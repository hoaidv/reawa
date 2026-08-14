---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-14
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "W11b done — EP-019 human PASS. W11c NOW — EP-025 chrome layers → /dev."
wave: W11c
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W11c**.

## Summary (as of 2026-08-14)

| Band | Count | Meaning |
|---|---|---|
| W0–W11a | **done** | Through selection-create (EP-018 human PASS) |
| **W11b** | **done** | EP-023 design done; EP-019 live manipulation (human PASS) |
| **W11c** | **NOW** | EP-025 selection chrome layers (CHL-0017) |
| W12 | planned | EP-020 ∥ IN-028 |

## Lock

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /dev EP-025
forbidden: nested enclose; regress EP-018 chip
NOW: W11c — STORY-EP-025 ready
cursor: /dev EP-025
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W11a** | **done** | — | EP-022 + EP-018 |
| **W11b** | **done** | serial | EP-023 (design) then EP-019 (implement) |
| **W11c** | **NOW** | serial after W11b | [STORY-EP-025](./stories/STORY-EP-025.md) ToolCanvasLayer / ToolLayer |
| W12 | planned | **∥ yes** | EP-020 ∥ IN-028 |

### Parallelism rules (W11c)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-025](./stories/STORY-EP-025.md) | `epaper/` canvas layers | do not clobber EP-018/019 behaviour |

## Full task table (delta)

| Id | Feature | Status | Wave | Next owner |
|---|---|---|---|---|
| F-08 | selection-create | **done** | W11a | — |
| F-09a | four-tool rebase | **done** | **W11b** | — |
| F-09 | manipulation | **done** | **W11b** | — |
| F-09b | chrome layers | **ready** | **W11c** | `/dev` |

## Verdict

**Next: `/dev` on EP-025.**
