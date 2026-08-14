---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-14
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "W11c done — EP-025 human PASS. W12 NOW — EP-020 ∥ IN-028 → /qa then /dev."
wave: W12
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W12**.

## Summary (as of 2026-08-14)

| Band | Count | Meaning |
|---|---|---|
| W0–W11c | **done** | Through chrome layers (EP-025 human PASS; CHL-0018 option 1) |
| **W12** | **NOW** | EP-020 device sync ∥ IN-028 desktop `doc_load` |

## Lock

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /qa then /dev EP-020 ∥ IN-028
forbidden: nested enclose; option-2 live node on CanvasLayer; regress EP-018/019/025
NOW: W12 — STORY-EP-020 ∥ STORY-IN-028 ready
cursor: /qa W12 BDD walk
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W11c** | **done** | serial | [STORY-EP-025](./stories/STORY-EP-025.md) ToolCanvasLayer |
| **W12** | **NOW** | **∥ yes** | [STORY-EP-020](./stories/STORY-EP-020.md) ∥ [STORY-IN-028](./stories/STORY-IN-028.md) |

### Parallelism rules (W12)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-020](./stories/STORY-EP-020.md) | `epaper/` session publish / handshake | wire seq; do not invent op type names |
| **B** | [STORY-IN-028](./stories/STORY-IN-028.md) | `infini/` `doc_load` handshake | same wire; drain before load |

Use SRS-IN-09 **transmit** names. No SmartGroup logic edits.

## Full task table (delta)

| Id | Feature | Status | Wave | Next owner |
|---|---|---|---|---|
| F-09b | chrome layers | **done** | W11c | — |
| F-10 | device one-way sync | **ready** | **W12** | `/qa` then `/dev` |
| F-11 | desktop `doc_load` | **ready** | **W12** | `/qa` then `/dev` |

## Verdict

**Next: `/qa` on EP-020 ∥ IN-028** (BDD walk), then **`/dev`** both lanes.
