---
title: Execution board — rebuild Epaper
iter: iter-005
track: TRACK-008
owner: sm
date: 2026-10-03
lock: vertical · verified · wip 1
verdict: "Shell story done on the host. Wait for the human to name the next slice. Follow-through stays paused."
wave: WAIT
---

# Execution board — rebuild Epaper

**Canonical board** for [TRACK-008](../tracks/TRACK-008-rebuild-epaper.md) (Rebuild Epaper).
[TRACK-007](../tracks/TRACK-007-follow-through.md) (Hand-on-paper follow-through) is **paused**.

## Summary (as of 2026-10-03)

| Band | Count | Meaning |
|---|---|---|
| Wave **NOW** | 0 | Shell landed. Waiting for the human to name the next slice |
| Paused | TRACK-007 | Do not start its stories |
| Implement freeze | none | Stop line is verified; this shell is the only in-flight story |

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 1
out_of_scope: backlog
modules: epaper
features: (1) epaper/local-pen-ink
personas: Developer for STORY-EP-081 only
forbidden: product documents; infini/; deleting epaper_old/; follow-through stories; a second feature
```

## Execution map

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0-shell** | **done** | serial | Archive `epaper/` to `epaper_old/` and add an empty reMarkable 2 shell |
| **Frozen** | paused | — | [TRACK-007](../tracks/TRACK-007-follow-through.md) remainder |

### Parallelism rules (current wave)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-081](./stories/STORY-EP-081.md) | `epaper/`, `epaper_old/`, `.gitignore` | one writer only |

### Full task table

| ID | Title | Status | Owner | Progress Detail |
|---|---|---|---|---|
| [STORY-EP-081](./stories/STORY-EP-081.md) | Empty reMarkable 2 shell | done | Developer | Host binary is ELF 32-bit ARM. Not launched on a tablet. `QT_QUICK_BACKEND=epaper` is set and Qt Quick is not linked. Git still shows the archive as untracked until a commit. |
| [TRACK-007](../tracks/TRACK-007-follow-through.md) | Hand-on-paper follow-through | paused | Scrum Master | Frozen 2026-10-03. Field latency stories were ready and not started. Do not resume until the human says the shell era is over. |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| A | Developer | [STORY-EP-081](./stories/STORY-EP-081.md) (Empty reMarkable 2 shell) | `epaper/**`, `epaper_old/**`, `.gitignore` | Archive intact; new tree is empty Qt main plus build ceremony; ARM binary or an explicit build blocker |

## Verdict

[STORY-EP-081](./stories/STORY-EP-081.md) (Empty reMarkable 2 shell) is **done** on the host. Do not spawn another lane until the human names the next slice. [TRACK-007](../tracks/TRACK-007-follow-through.md) (Hand-on-paper follow-through) stays paused.
