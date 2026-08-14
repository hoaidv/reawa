---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-14
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "FROZEN — W12 done, campaign exited 2026-08-14. See retro."
wave: closed
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W12 complete**.

## Summary (as of 2026-08-14)

| Band | Count | Meaning |
|---|---|---|
| W0–W12 | **done** | Device document, ink-box, manip, one-way sync shipped and human-confirmed |
| **NOW** | **PM + human** | Campaign exit: `/pm` gate-close W12; human verify REQ-04…07 |

## Lock

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /pm (gate) then human campaign verify
forbidden: nested enclose (CHL-0011); FREE_FORM/align-content (CHL-0012); REQ-08 node manip; new implement stories past W12
NOW: W12 complete — STORY-EP-020 ∥ STORY-IN-028 done
cursor: /pm W12 gate + human verify of campaign exit criteria
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W11c** | **done** | serial | [STORY-EP-025](./stories/STORY-EP-025.md) ToolCanvasLayer |
| **W12** | **done** | **∥ yes** | [STORY-EP-020](./stories/STORY-EP-020.md) ∥ [STORY-IN-028](./stories/STORY-IN-028.md) |

No further implement wave in this lock. Residue EP-007…011 / IN-020…026 stays **blocked**.

## Full task table (delta)

| Id | Feature | Status | Wave | Next owner |
|---|---|---|---|---|
| F-09b | chrome layers | **done** | W11c | — |
| F-10 | device one-way sync | **done** | W12 | `/pm` |
| F-11 | desktop `doc_load` | **done** | W12 | `/pm` |

## Parking lot (not W12, not a new wave)

| Item | Status |
|---|---|
| ADR-0019 amend for CHL-0018 (option-2 live node) | deferred — does not gate this campaign |
| CHL-0011 nested enclose | future |
| CHL-0012 FREE_FORM / align-content | future |
| epaper `[REQ-08]` any-node manip | next campaign |

## Ops note

Deploy must set `RM_SYNC_HOST=<Mac USB IP>` (usually `10.11.99.12`). Without it, Infini stays “RM disconnected.”

## Verdict

**Next: `/pm`** — gate-close EP-020 and IN-028, then human campaign verify (stop_line `verified`: REQ-04…07). Do not open iter-004. Do not slice REQ-08 here.
