---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-11
lock: vertical · verified · 2 features · WIP 2
verdict: "READY — W3 NOW: designer IN-013 ∥ qa→dev IN-012 + EP-004"
wave: W3
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md).

## Summary (as of 2026-08-11)

| Band | Count | Meaning |
|---|---|---|
| W0–W2 | **done** | PM thicken + arch confirm + SM slice |
| **W3 NOW** | 3 | IN-013 design ∥ IN-012 ingest ∥ EP-004 spike |
| Design front | 0/2 done | IN-013 ready; EP-003 draft (spike-gated) |
| Later implement | 7 draft/ready | undo ready; UI stories draft |

## Lock

```
direction: vertical
stop_line: verified
autonomy: bounded
out_of_scope: backlog
wip: 2
modules: infini, epaper
features: (2) infini/vector-document; epaper/tool-modes
personas: /designer NOW (IN-013); /qa → /dev NOW (IN-012, EP-004); then undo→selection→enclose
forbidden: /dev UI stories before design done; epaper design before EP-004 spike; DocChrome; silent Must
NOW: /designer + /qa (then /dev) in parallel
cursor: /designer ∥ /qa
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| W0–W1 | **done** | — | PM + arch confirm |
| W2 | **done** | — | SM slice (this board) |
| **W3** | **NOW** | **∥ yes** | A: IN-013 design · B: IN-012 ingest · C: EP-004 spike |
| W4 | next | serial after W3a+c | IN-014 undo → IN-015 selection → IN-010 enclose → IN-016/017 |
| W5 | after spike | serial | EP-003 design → IN-018 transport → EP-005 tools |

### Parallelism rules (W3)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | STORY-IN-013 | `design/ink-box-ui/` | none vs B/C |
| **B** | STORY-IN-012 | `infini/src/` ink path | do not parallel-edit same files as later W4 without stitch |
| **C** | STORY-EP-004 | `epaper/` spike notes | none vs A/B |

## Full task table

| Id | Feature / chore | Pri | Status | Wave | Next owner | ∥ |
|---|---|---|---|---|---|---|
| STORY-IN-012 | Tree-backed ink ingestion | P0 | **ready** | W3 | `/qa`→`/dev` | B |
| STORY-EP-004 | RM2 touch spike | P0 | **ready** | W3 | `/qa`→`/dev` | C |
| STORY-IN-013 | Design Infini ink-box | P0 | **ready** | W3 | `/designer` | A |
| STORY-IN-014 | Undo ring | P0 | ready | W4 | `/qa`→`/dev` | after B |
| STORY-IN-015 | Selection + fixedInk UV | P0 | draft | W4 | `/qa`→`/dev` | after A+B |
| STORY-IN-010 | Tool-armed enclose | P0 | draft | W4 | `/qa`→`/dev` | after 015 |
| STORY-IN-016 | Draw-into membership | P0 | draft | W4 | `/qa`→`/dev` | after 010 |
| STORY-IN-017 | Surround selection create | P0 | draft | W4 | `/qa`→`/dev` | after A+B |
| STORY-EP-003 | Design epaper strip | P0 | draft | W5 | `/designer` | after C |
| STORY-IN-018 | Tool intent transport | P1 | draft | W5 | `/qa`→`/dev` | after 010/015 |
| STORY-EP-005 | Epaper tool modes | P1 | draft | W6 | `/qa`→`/dev` | after EP-003+018 |

## Parking lot

| Item | Sink |
|---|---|
| `doc_op` migration | backlog |
| Rotation + connectors on Smart Group | backlog |
| Mouse ink on Infini | backlog |
| In-box alignment | backlog |
| DocChrome (IN-006) | cancelled |

## Verdict

Slice complete. **Next: `/designer`** on IN-013 **and** **`/qa`** (then `/dev`) on IN-012 + EP-004 — no pause between QA BDD and Dev.
