---
title: Execution board — hand-on-paper
iter: iter-005
track: TRACK-005
owner: sm
date: 2026-08-19
lock: vertical · verified · wip 2
verdict: "W1 design done (EP-037 ∥ IN-034). NOW W2 Quality Assurance Engineer behavior-driven scenarios for EP-038 after human visual check. Not ready for Developer."
wave: W2
---

# Execution board — hand-on-paper

**Canonical board** for [TRACK-005](../tracks/TRACK-005-hand-on-paper.md).

---

## Summary (as of 2026-08-19)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 2 | [UI-EP-06](../../.docs/design/index.md) hand-touch · [UI-IN-03](../../.docs/design/index.md) pen-button map |
| Wave **NOW** | W2 | Quality Assurance Engineer behavior-driven scenarios for [STORY-EP-038](./stories/STORY-EP-038.md) (One-finger hit box: select freeform and move) — after human glance at packages |
| Next | W2 implement | Developer EP-038 after scenarios exist |

Design front target: **2/7** packages.

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
personas: /qa then /dev EP-038
forbidden: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; do not /dev until BDD for the story
NOW: W2 /qa BDD EP-038
cursor: /qa STORY-EP-038 then /dev STORY-EP-038
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** 2026-08-19 | serial | Solution Architect Software Requirements Specification + Architecture Decision Records |
| **W1** | **done** 2026-08-19 | **∥** | Product Designer EP-037 **∥** IN-034 |
| **W2** | **NOW** | serial | Quality Assurance Engineer then Developer EP-038 (one-finger). Human visual check of design packages first. |
| **W3** | queued | ∥ after W1 | erase EP-040 → 041/042; clipboard EP-043 → 044 |
| **W4** | queued | | connector ends 045–047 then attachments 048–049 |
| **W5** | queued | | barrel EP-052 + IN-035; then two-finger EP-039 + IN-033 (BRD-07 gate) |
| **W6** | queued | | manual create EP-050–051 (Should) |
| **Frozen** | — | | REQ-15 · REQ-08 · EP-035 parking |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | Quality Assurance Engineer EP-038 | `epaper/ink-box/bdd/**` (and related) | serial before Developer; do not implement yet |

Work-in-progress 2 does not mean two implement stories. Wave 2 is serial: scenarios then code for one-finger only.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group | Progress Detail |
|---|---|---|---|---|---|---|---|---|---|
| F-10 | REQ-10 hand-touch | Must | SRS bound | [STORY-EP-037](./stories/STORY-EP-037.md) **done** | W2 next | W2 | Quality Assurance Engineer | A | Package [hand-touch](./design/hand-touch/index.html). Open [CHL-0022](./challenges/CHL-0022-shipped-no-device-pan.md). Two-finger **ship** still BRD-07 (Wave 5) even though EP-039 has scene links. |
| F-11 | REQ-11 erase | Must | SRS bound | [STORY-EP-040](./stories/STORY-EP-040.md) draft | queued | W3 | designer later | — | Do not open a third design package until W2 one-finger is through or human raises work-in-progress. |
| F-12 | REQ-12 clipboard | Must | SRS bound | [STORY-EP-043](./stories/STORY-EP-043.md) draft | queued | W3 | designer later | — | ADR-0024 proposed. |
| F-13 | REQ-13 ends | Must | SRS bound | [STORY-EP-045](./stories/STORY-EP-045.md) draft | queued | W4 | designer later | — | ADR-0026 proposed. |
| F-14 | REQ-14 attachments | Must | SRS bound | [STORY-EP-048](./stories/STORY-EP-048.md) draft | queued | W4 | designer later | — | ADR-0027 accepted. |
| F-17 | REQ-17 manual create | Should | SRS bound | [STORY-EP-050](./stories/STORY-EP-050.md) draft | queued | W6 | designer later | — | Specified in W0. |
| F-18 | REQ-18 + Infini REQ-05 buttons | Must | SRS bound | [STORY-IN-034](./stories/STORY-IN-034.md) **done** | queued implement | W5 | Quality Assurance Engineer later | W1-B done | Package [pen-button-map](./design/pen-button-map/index.html). IN-035 / EP-052 have spec links; stay draft. |
| CHORE-1 | BRD-07 pan/zoom amend | — | — | [STORY-EP-039](./stories/STORY-EP-039.md) | open | W5 | analyst | — | SRS-EP-24 + design scenes exist; ship gated. |
| CHORE-2 | PM adopt CHL-0022 + PRD comments + accept ADR-0023…26 + optional experience thicken | — | — | — | open | after W0 | product-manager | — | Not a W2 blocker for one-finger. |
| PL-035 | EP-035 enclose A/L | P1 | — | — | ready | parking | Developer later | — | Not this wave. |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Writes | Done when |
|---|---|---|---|---|
| A | Quality Assurance Engineer | STORY-EP-038 One-finger hit box: select freeform and move | feature `bdd/` tagged `@SRS-EP-21` / `@SRS-EP-23` / `@SRS-EP-25` | scenarios cover story acceptance; then Developer may start |

### Backlog sink

| Item | Why |
|---|---|
| REQ-15 tables | human excluded |
| REQ-08 / CHL-0011 / CHL-0012 | parked |
| EP-032 chrome SM | parked iter-004 |

---

## Verdict

**W1 complete.** Human should open the two package navigators, then Scrum Master spawns Quality Assurance Engineer for [STORY-EP-038](./stories/STORY-EP-038.md). Do not start Developer until those scenarios exist. Do not spawn erase/clipboard design in parallel unless the human raises the work-in-progress cap.
