---
title: Execution board — document forest
iter: iter-006
track: TRACK-009
owner: sm
date: 2026-10-04
lock: vertical · verified · wip 1 · ask
verdict: "STORY-EP-082 ready. The human codes it; agents guide and review."
wave: A1
---

# Execution board — document forest

**Canonical board** for [TRACK-009](../tracks/TRACK-009-document-forest.md) (Document forest).
Plan: [document-forest-implementation.md](../document-forest-implementation.md).

## Summary (as of 2026-10-04)

| Band | Count | Meaning |
|---|---|---|
| Wave **NOW** | 1 | [STORY-EP-082](./stories/STORY-EP-082.md) ready |
| Committed, waiting | 5 | EP-083…087, each `ready` when its predecessor is done |
| Planned | 4 | EP-088…091, not committed this iter |
| Paused | TRACK-007 | Do not start its stories |

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: ask
wip: 1
out_of_scope: backlog
modules: epaper
features: epaper/device-document (+ epaper/local-pen-ink for STORY-EP-085 only)
personas: the human writes code; agents guide, review and check evidence
forbidden: agent edits under epaper/src/; infini/ and the sync wire (CHL-0034); recognizers and tools; product chrome; deleting epaper_old/
```

## Execution map

| Wave | Stories | Status | What you can show |
|---|---|---|---|
| **A1** | EP-082, EP-083 | **NOW** | Host tests in three presets; slot table and handles |
| **A2** | EP-084, EP-085 | waiting | Nodes and children; pen ink painted from the forest on the device |
| **B** | EP-086, EP-087 | waiting | Per-container R-tree; readers beside the writer, sanitizers clean |
| **C–F** | EP-088…091 | planned | Manipulation, connector, progressive paint, undo |

## Full task table

| ID | Title | Status | Owner | Progress detail |
|---|---|---|---|---|
| [STORY-EP-082](./stories/STORY-EP-082.md) | Forest foundations: host tests and geometry | ready | human | Not started |
| [STORY-EP-083](./stories/STORY-EP-083.md) | Slot table, handles and NodeStore | draft | human | Waits on EP-082 |
| [STORY-EP-084](./stories/STORY-EP-084.md) | Nodes, children, bounds and invariants | draft | human | Waits on EP-083 |
| [STORY-EP-085](./stories/STORY-EP-085.md) | Milestone A: paint pen ink from the forest | draft | human | Waits on EP-084 |
| [STORY-EP-086](./stories/STORY-EP-086.md) | R-tree per container | draft | human | Waits on EP-085 |
| [STORY-EP-087](./stories/STORY-EP-087.md) | Concurrency: granules, commit, reclamation | draft | human | Waits on EP-086 |
| [STORY-EP-088](./stories/STORY-EP-088.md) | Manipulation core | draft | human | Planned |
| [STORY-EP-089](./stories/STORY-EP-089.md) | Connector on the forest | draft | human | Planned |
| [STORY-EP-090](./stories/STORY-EP-090.md) | Progressive paint | draft | human | Planned |
| [STORY-EP-091](./stories/STORY-EP-091.md) | Undo and redo on the forest | draft | human | Planned |

## Verdict

Step 0 of the plan is done. [STORY-EP-082](./stories/STORY-EP-082.md) (Forest foundations) is
ready for the human.
