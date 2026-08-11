---
title: Execution board — Infini campaign
iter: iter-002
track: TRACK-002
owner: sm
date: 2026-08-11
lock: vertical · verified · 4 feature(s) · wip 1
verdict: ""
wave: W5
---

# Execution board — Infini ↔ Epaper

## Summary (as of 2026-08-11)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 1 | STORY-IN-001 (F1 only) |
| Feature **verified** | 1 | F1 infinity-canvas |
| Wave **done** | W4 | Must tree/SVG/session + human RM→Infini draw |
| Wave **NOW** | **W5** | Live pan/zoom → tablet (IN-011 → EP-002) |
| Design cancelled | 1 | STORY-IN-006 blocked |
| Implement **ready** | 2 | IN-011, EP-002 |
| Implement draft | 1 | IN-010 Smart Group Could parked |

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
out_of_scope: backlog
wip: 1
modules: infini, epaper
features: (4) infini/infinity-canvas; infini/vector-document; infini/tablet-sync; epaper/region-sync
personas: /dev (NOW IN-011→EP-002); /qa verify after; /sm maintains board
forbidden: reawa/*; epaper on-device pan/zoom; /designer on IN-006; revive DocChrome; IN-010
NOW: W5 implement — STORY-IN-011 then STORY-EP-002
cursor: /dev
```

## Execution map

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| W0–W3 | done | — | PRD/ADR → design F1 → impl F1 → arch sync bind |
| **W4** | **done** | — | IN-007…009 + EP-001; hardware draw confirmed |
| **W5** | **NOW** | serial (wip 1) | Viewport live wire: marker + publish + e-ink refresh + stroke scale |

### Parallelism rules (current wave)

| Lane | Work | Writes | Conflicts |
|---|---|---|---|
| **A** | Architect → QA BDD → Dev | `.docs/**/srs-*`, bdd, `infini/`, `epaper/` | honor depends_on; one story in flight |

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F1 | infini/infinity-canvas | Must | ok | IN-001 done | **done** | W2 | — | — |
| F2 | infini/vector-document | Must | ok | IN-006 cancelled | **done** (W4) | W4 | — | — |
| F2b | Smart Group pilot (REQ-04) | Could | ADR-0011 | n/a | draft IN-010 | park | — | — |
| F3 | infini/tablet-sync | Must | thick | n/a | **ready** IN-011 | W5 | **dev** | A |
| F4 | epaper/region-sync | Must | thick | n/a | **ready** EP-002 | W5 | **dev** | A |

### Story order (W5)

| Order | Story | SRS | Status | Depends |
|---|---|---|---|---|
| 1 | [STORY-IN-011](./stories/STORY-IN-011.md) | SRS-IN-07 | **ready** | IN-009 |
| 2 | [STORY-EP-002](./stories/STORY-EP-002.md) | SRS-EP-02 | **ready** | EP-001, IN-011 |
| — | [STORY-IN-010](./stories/STORY-IN-010.md) | SRS-IN-10 | draft Could | parked |

### Sub-agent roster (W5)

| Agent | Done-when |
|---|---|
| Architect | done (READY-WITH-CONCERNS) |
| QA | BDD authored; verify after Dev `in-review` |
| Dev | **NOW** IN-011 → EP-002 |

## Verdict

**W5 in flight.** BDD ready. Next: **`/dev`** IN-011 then EP-002 → `/qa` verify.
