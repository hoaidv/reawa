---
title: Execution board — hand-on-paper
iter: iter-005
track: TRACK-005
owner: sm
date: 2026-08-16
lock: vertical · verified · wip 2
verdict: "NOT-READY for /dev — W0 architect SRS; W1 design EP-037 ∥ IN-034"
wave: W0
---

# Execution board — hand-on-paper

**Canonical board** for [TRACK-005](../tracks/TRACK-005-hand-on-paper.md).

---

## Summary (as of 2026-08-16)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 0 | |
| Wave **NOW** | W0 `/architect` | bind SRS for REQ-10…14, 17, 18 + infini REQ-05 |
| Next ∥ | EP-037 ∥ IN-034 | `/designer` after W0 |

Design front target: **0/7** packages.

---

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
out_of_scope: backlog
modules: epaper, infini
features: epaper/ink-box; epaper/tool-modes; epaper/connector-ink; epaper/region-sync; epaper/local-pen-ink; epaper/device-document; infini/infinity-canvas; infini/tablet-sync; infini/vector-document
personas: /architect now; then /designer EP-037 ∥ IN-034
forbidden: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI
NOW: W0 SRS bind
cursor: /architect then /designer EP-037 ∥ IN-034
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **NOW** | serial | `/architect` SRS + ADRs (viewport last-writer, clipboard ops, button channel, endpoint-ink) |
| **W1** | next | **∥ yes** | `/designer` EP-037 hand-touch **∥** IN-034 pen-button map |
| **W2** | queued | serial | `/qa` then `/dev` EP-038 (1-finger) |
| **W3** | queued | ∥ after W1 | erase EP-040 → 041/042; clipboard EP-043 → 044 |
| **W4** | queued | | connector ends 045–047 then attachments 048–049 |
| **W5** | queued | | barrel EP-052 + IN-035; then 2-finger EP-039 + IN-033 (BRD-07) |
| **W6** | queued | | manual create EP-050–051 (P2) |
| **Frozen** | — | | REQ-15 · REQ-08 · EP-035 parking |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | architect | `.docs/modules/epaper/features/**`, `.docs/adr/**` | do not start design packages until SRS ids exist |

WIP 2 applies to **W1** (two design packages). W0 is serial.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-10 | REQ-10 hand-touch | Must | PRD thick / SRS thin | EP-037 | draft | W0→W1 | `/architect` | A then W1-A |
| F-11 | REQ-11 erase | Must | PRD | EP-040 | draft | W3 | `/architect` | — |
| F-12 | REQ-12 clipboard | Must | PRD | EP-043 | draft | W3 | `/architect` | — |
| F-13 | REQ-13 ends | Must | PRD | EP-045 | draft | W4 | `/architect` | — |
| F-14 | REQ-14 attachments | Must | PRD | EP-048 | draft | W4 | `/architect` | — |
| F-17 | REQ-17 manual create | Should | PRD | EP-050 | draft | W6 | `/architect` | — |
| F-18 | REQ-18 + IN-05 buttons | Must | PRD | IN-034 | draft | W0→W1 | `/architect` | W1-B |
| CHORE-1 | BRD-07 pan/zoom amend | — | — | EP-039 | open | W5 | `/analyst` | — |
| PL-035 | EP-035 enclose A/L | P1 | — | — | ready | parking | `/dev` later | — |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Writes | Done when |
|---|---|---|---|---|
| A | architect | REQ-10…18 except 15 | feature SRS + ADRs | stories can retarget parent_srs |

### Backlog sink

| Item | Why |
|---|---|
| REQ-15 tables | human excluded |
| REQ-08 / CHL-0011 / CHL-0012 | parked |
| EP-032 chrome SM | parked iter-004 |

---

## Verdict

**NOT-READY** for `/designer` paint until W0 SRS exists (stories currently hang off nearest old SRS ids). After W0: **`/designer` EP-037 ∥ IN-034**. Do not `/dev` until design `done` + `/qa` BDD.
