---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-13
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "W11a NOW — EP-022 design ready (selection-enclose). EP-018 draft until design done. W10 done."
wave: W11a
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W11a design**.

## Summary (as of 2026-08-13)

| Band | Count | Meaning |
|---|---|---|
| W0–W9 | **done** | Through undo + tree + applier |
| **W10** | **done** | EP-016 enclose; EP-017 membership; Device Log |
| **W11a** | **NOW** | EP-022 design (rubber-band + Enclose CTA) → then EP-018 implement |
| W11b | planned | EP-019 live manipulation (28/56/96) |
| W12 | planned | EP-020 ∥ IN-028 |
| CHL-0013 | **adopted** | selection-create chrome this campaign |
| CHL-0010 | deferred | undo chrome only (selection-create reopened via 0013) |
| CHL-0011/0012 | future | nesting; sizing/align |

## Lock

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /designer EP-022; then /qa→/dev EP-018
forbidden: /dev EP-018 before EP-022 done; fourth ToolChip; nested enclose; undo chrome invent
NOW: W11a — STORY-EP-022 ready
cursor: /designer
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W10** | **done** | — | Enclose + membership + Device Log |
| **W11a** | **NOW** | serial | EP-022 design → EP-018 implement |
| W11b | planned | after 11a or ∥ if capacity | EP-019 manipulation |
| W12 | planned | **∥ yes** | EP-020 ∥ IN-028 |

### Parallelism rules (W11a)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **G** | [STORY-EP-022](./stories/STORY-EP-022.md) | `design/selection-enclose-chrome/` | compose UI-EP-02; no epaper/ code |
| — | EP-018 | blocked on G | no `/dev` yet |

## Full task table (delta)

| Id | Feature | Status | Wave | Next owner |
|---|---|---|---|---|
| F-07 | membership | **done** | W10 | — |
| F-08 | selection-create | design **ready** / impl draft | **W11a** | `/designer` EP-022 |
| F-09 | manipulation | draft | W11b | after EP-018 or parallel |
| CHL-0013 | rubber-band + Enclose | **adopted** | W11a | designer |

## Verdict

**Next: `/designer` on EP-022.** After design `done`, SM flips EP-018 → `ready` → `/qa` → `/dev`.
