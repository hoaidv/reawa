---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-14
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "W11b in-review — EP-019 host PASS → /qa."
wave: W11b
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W11b**.

## Summary (as of 2026-08-14)

| Band | Count | Meaning |
|---|---|---|
| W0–W11a | **done** | Through selection-create (EP-018 human PASS) |
| **W11b** | **NOW** | EP-023 design done; EP-019 live manipulation (28/56/96 du, both selection tools) |
| W12 | planned | EP-020 ∥ IN-028 |

## Lock

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /dev EP-019
forbidden: nested enclose; regress EP-018 chip
NOW: W11b — STORY-EP-019 ready
cursor: /qa EP-019
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W11a** | **done** | — | EP-022 + EP-018 |
| **W11b** | **NOW** | serial | EP-023 (design) then EP-019 (implement) |
| W12 | planned | **∥ yes** | EP-020 ∥ IN-028 |

### Parallelism rules (W11b)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-019](./stories/STORY-EP-019.md) | `epaper/` + UI-EP-02 | do not clobber EP-018 selection-create |

## Full task table (delta)

| Id | Feature | Status | Wave | Next owner |
|---|---|---|---|---|
| F-08 | selection-create | **done** | W11a | — |
| F-09a | four-tool rebase | **done** | **W11b** | — |
| F-09 | manipulation | **in-review** | **W11b** | `/qa` |

## Verdict

**Next: `/qa` on EP-019** (host + on-panel).
