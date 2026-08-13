---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-13
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "W8 NOW — /designer EP-012 ∥ /dev EP-013; CHL-0009 blocks document/sync implement"
wave: W8
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W8**.

## Summary (as of 2026-08-13)

| Band | Count | Meaning |
|---|---|---|
| W0–W6 | **done** | Pilot slice at git HEAD — kept as history; its ownership model is withdrawn |
| W7 verify-fix | **void** | Superseded by CHL-0008 (code restored; no patch wave) |
| Patch-wave residue | **blocked** | EP-007…011, IN-020…026 — assumed desktop authority |
| Architecture rework | **done** | ADR-0014, ADR-0015, domain doc, SRS-EP-07…14, BDD, architecture views |
| W8 NOW | **2 ready** | EP-012 design ∥ EP-013 latency |
| W9–W12 | **sliced** | document → recognition → manipulation → sync; desktop applier first |
| CHL-0009 | **open** | missing `device-document/srs-logic.md` — blocks EP-014/015/020 |

## Lock (re-locked 2026-08-13, CHL-0008 adopted)

```
direction: vertical
stop_line: verified
autonomy: bounded
out_of_scope: backlog
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /designer NOW (EP-012) ∥ /dev NOW (EP-013) → /architect (CHL-0009) → /qa → /dev W9+
forbidden: /dev on any story assuming desktop tree authority; any design reintroducing a
           peer round trip inside an editing gesture; /dev on REQ-04 implement before EP-013 passes
NOW: /designer STORY-EP-012 ∥ /dev STORY-EP-013
cursor: W8 — device-selection-chrome + ink-latency gate
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| W0–W6 | **done** | — | Shipped at HEAD (pilot model, now withdrawn) |
| W7 | **void** | — | EP-006 ∥ IN-019 verify-fix — do not pull |
| Rework design | **done** | — | ADR-0014 + ADR-0015 + domain doc + SRS-EP-07…14 + BDD |
| **W8** | **NOW** | **∥ yes** | EP-012 design ∥ EP-013 latency measurement |
| W9 | next | **∥ yes** | EP-014 → EP-015 (device document) ∥ IN-027 (desktop applier) |
| W10 | planned | serial on device | EP-016 enclose → EP-017 membership → EP-018 selection-create |
| W11 | planned | serial | EP-019 live manipulation + REQ-08 conformance (needs EP-012 done) |
| W12 | planned | **∥ yes** | EP-020 device sync ∥ IN-028 desktop `doc_load` |
| Frozen | — | — | REQ-08 node-manipulation (next campaign); infini REQ-04 authoring |

### Parallelism rules (W8)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-012](./stories/STORY-EP-012.md) | `.plan/iter-003/design/device-selection-chrome/` | do not edit `epaper/` code |
| **B** | [STORY-EP-013](./stories/STORY-EP-013.md) | `epaper/` paint path + measurement harness | do not edit the design package |

Lanes **A** and **B** may run simultaneously. Do not start W9 REQ-04 implement until B passes.

### Current-wave sub-agent roster

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| A | designer | [STORY-EP-012](./stories/STORY-EP-012.md) | `design/device-selection-chrome/` | `ui-spec-gate` + story `done`; spike answers recorded |
| B | dev | [STORY-EP-013](./stories/STORY-EP-013.md) | device paint path (probe only) | p95 ≤30 ms recorded, or `CHL-*` filed |

## Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/ink-box chrome | Must | thick | [EP-012](./stories/STORY-EP-012.md) | ready | **W8** | `/designer` | A |
| F-02 | epaper/device-document latency | Must | thick | — | ready | **W8** | `/dev` | B |
| F-03 | epaper/device-document tree | Must | **gap** (CHL-0009) | — | blocked | W9 | `/architect` then `/dev` | C |
| F-04 | epaper/device-document undo | Must | **gap** (CHL-0009) | — | blocked | W9 | `/dev` | C (after F-03) |
| F-05 | infini/tablet-sync applier | Must | thick | — | draft | W9 | `/dev` | D ∥ C |
| F-06 | epaper/ink-box enclose | Must | thick | — | draft | W10 | `/dev` | — |
| F-07 | epaper/ink-box membership | Must | thick | — | draft | W10 | `/dev` | — |
| F-08 | epaper/ink-box selection-create | Must | thick | EP-012 | draft | W10 | `/dev` | — |
| F-09 | epaper/ink-box manipulation | Must | thick | EP-012 | draft | W11 | `/dev` | — |
| F-10 | epaper/device-document sync | Must | **gap** (CHL-0009) | — | blocked | W12 | `/dev` | E |
| F-11 | infini/tablet-sync `doc_load` | Must | thick | — | draft | W12 | `/dev` | E ∥ |
| CHL-0008 | architecture rework | — | — | — | resolved / adopted | — | — | — |
| CHL-0009 | missing srs-logic.md | high | — | — | **open** | W8 | `/pm` → `/architect` | serial vs F-03 |
| STORY-EP-006 / IN-019 | verify-fix | — | — | — | done (parked) | W7 void | — | — |
| EP-007…011, IN-020…026 | residue | — | — | — | blocked | — | — | — |

## Feature matrix

| Feature | Design / pipeline | Next |
|---|---|---|
| epaper/ink-box | W8 design in flight | EP-012 then W10/W11 implement |
| epaper/device-document | latency NOW; logic SRS missing | EP-013 then CHL-0009 then W9 |
| epaper/tool-modes | EP-003/005 done; compose, don't redo | — |
| infini/tablet-sync | applier sliced | W9 IN-027 |

## Residual TBDs (do not block W8)

| Area | TBD |
|---|---|
| Handle size, LOD cutoff, undo affordance, selection-create invocation | EP-012 spike |
| `srs-logic.md` for SRS-EP-07/08 | CHL-0009 |

## Backlog sink (do not schedule)

- epaper `[REQ-08]` node-manipulation — next campaign
- Desktop-side ink-box authoring (infini `[REQ-04]`) — until multi-directional sync
- On-device persistence, CRDT/OT, multi-document

## Verdict

**READY-WITH-CONCERNS** — re-slice is in; W8 can start. Concern: [CHL-0009](./challenges/CHL-0009-missing-device-document-srs-logic.md)
blocks W9 document/sync until `/architect` drops `srs-logic.md`. EP-013 fail would stop the campaign.
No `/dev` on W9+ until the latency gate passes. Do not pull residue EP-007…011 / IN-020…026.
