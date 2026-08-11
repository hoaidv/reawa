---
title: Execution board — Infini campaign
iter: iter-002
track: TRACK-002
owner: sm
date: 2026-08-11
lock: vertical · verified · 4 features · wip 1
verdict: ""
wave: W4
---

# Execution board — Infini ↔ Epaper

## Summary (as of 2026-08-11)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 1 | STORY-IN-001 (F1 only) |
| Feature **verified** | 1 | F1 infinity-canvas |
| Wave **done** | W3-arch | Architect sync-ready (READY-WITH-CONCERNS) |
| Wave **NOW** | **W4** | Implement vector-doc + tablet/region sync |
| Design cancelled | 1 | STORY-IN-006 blocked |
| Implement ready | 4 | IN-007…009, EP-001 → `/qa` BDD then `/dev` |
| Implement draft | 1 | IN-010 Smart Group Could |

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
out_of_scope: backlog
wip: 1
modules: infini, epaper
features: (4) infini/infinity-canvas; infini/vector-document; infini/tablet-sync; epaper/region-sync
personas: /qa (NOW BDD); /dev after BDD; /sm maintains board
forbidden: reawa/*; epaper on-device pan/zoom; /designer on IN-006; revive DocChrome design
NOW: W4 implement slices — start STORY-IN-007 (tree/ops); sync path IN-008→009 + EP-001; IN-010 after
cursor: /qa
```

## Execution map

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| W0 | done | — | PRD + ADR |
| W1 | done | — | `/designer` STORY-IN-001 |
| W2 | done | — | `/dev`+`/qa` STORY-IN-002…005 |
| W3-arch | **done** | — | `/architect` document↔sync bind |
| **W4** | **NOW** | serial (wip 1) | implement vector-doc + Smart Group **with** tablet/region sync |

### Parallelism rules (current wave)

| Lane | Work | Writes | Conflicts |
|---|---|---|---|
| **A** | QA BDD then Dev | `.docs/**/bdd/`, `infini/`, `epaper/` | honor depends_on; one story in flight |

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F1 | infini/infinity-canvas | Must | ok | IN-001 done | **done** | W2 | — | — |
| F2 | infini/vector-document | Must | SRS/ADR ready | IN-006 **cancelled** | **impl** | W4 | **qa→dev** | A |
| F2b | Smart Group pilot (REQ-04) | Could | ADR-0011 | n/a | draft IN-010 | W4 | qa→dev (after Must) | A |
| F3 | infini/tablet-sync | Must | thick | n/a | **impl** IN-009 | W4 | qa→dev | A |
| F4 | epaper/region-sync | Must | thick | n/a | **impl** EP-001 | W4 | qa→dev | A |

### Story order (W4)

| Order | Story | SRS | Status | Depends |
|---|---|---|---|---|
| 1 | [STORY-IN-007](./stories/STORY-IN-007.md) | SRS-IN-04 | **ready** | — |
| 2 | [STORY-IN-008](./stories/STORY-IN-008.md) | SRS-IN-09 | **ready** | IN-007 |
| 3 | [STORY-IN-009](./stories/STORY-IN-009.md) | SRS-IN-07 | **ready** | IN-007, IN-008 |
| 4 | [STORY-EP-001](./stories/STORY-EP-001.md) | SRS-EP-02 | **ready** | IN-009 |
| 5 | [STORY-IN-010](./stories/STORY-IN-010.md) | SRS-IN-10 | **draft** | IN-007, IN-009 |

### Sub-agent roster (W4)

| Agent | Done-when |
|---|---|
| QA | BDD for IN-007…009 + EP-001 (and IN-010 when unblocked); hand `/dev` |
| Dev | Implement in depends_on order; verify to stop_line |

## Verdict

**W3-arch closed.** **W4 open** with implement slices (no DocChrome design). Next: **`/qa`**
BDD for the Must chain, then **`/dev`** on STORY-IN-007. Reconnect snapshot/hello remains TBD
(architect concern) — do not block Must ACs.
