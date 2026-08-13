---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-13
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "W9 auto — EP-015 QA walk in flight, then /dev; ring no chrome"
wave: W9
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **active, W9**.

## Summary (as of 2026-08-13)

| Band | Count | Meaning |
|---|---|---|
| W0–W6 | **done** | Pilot slice at git HEAD — kept as history; its ownership model is withdrawn |
| W7 verify-fix | **void** | Superseded by CHL-0008 (code restored; no patch wave) |
| Patch-wave residue | **blocked** | EP-007…011, IN-020…026 — assumed desktop authority |
| Architecture rework | **done** | ADR-0014, ADR-0015, domain doc, SRS-EP-07…14, BDD, architecture views |
| W8 NOW | **done** | EP-012 design + EP-013 latency (device pass) |
| **W9 NOW** | **EP-015 ready** | EP-014 **done** (RM2); IN-027 **done**; undo ring next |
| W10–W12 | **sliced** | recognition → manipulation → sync |
| CHL-0009 | **resolved** | `srs-logic.md` landed |
| CHL-0010 | **deferred** | no on-panel undo / no selection-create CTA; EP-018 frozen draft |

## Lock (re-locked 2026-08-13, CHL-0008 adopted)

```
direction: vertical
stop_line: verified
autonomy: bounded
out_of_scope: backlog
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /qa then /dev EP-015 (SM auto)
forbidden: /dev on any story assuming desktop tree authority; any design reintroducing a
           peer round trip inside an editing gesture; residue EP-007…011 / IN-020…026;
           on-panel undo chrome (CHL-0010); EP-020 handshake; Infini edits
NOW: /qa STORY-EP-015 (review undo-ring.feature) then /dev — auto
cursor: W9 auto — EP-015 QA in flight; ring with no chrome; do not start W10
```

## Waves

| Wave | Status | Parallel? | What |
|---|---|---|---|
| W0–W6 | **done** | — | Shipped at HEAD (pilot model, now withdrawn) |
| W7 | **void** | — | EP-006 ∥ IN-019 verify-fix — do not pull |
| Rework design | **done** | — | ADR-0014 + ADR-0015 + domain doc + SRS-EP-07…14 + BDD |
| **W8** | **done** | — | EP-012 `[UI-EP-02]` + EP-013 latency (RM2 pass) |
| **W9** | **NOW** | serial on device | EP-014 **done**; IN-027 **done**; **EP-015** undo ring |
| W10 | planned | serial on device | EP-016 enclose → EP-017 membership; **EP-018 frozen** (CHL-0010) |
| W11 | planned | serial | EP-019 live manipulation (28/56/96 du locked) + REQ-08 conformance |
| W12 | planned | **∥ yes** | EP-020 device sync ∥ IN-028 desktop `doc_load` |
| Frozen | — | — | REQ-08 node-manipulation (next campaign); infini REQ-04 authoring |

### Parallelism rules (W9)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **C** | [STORY-EP-015](./stories/STORY-EP-015.md) | `epaper/` undo ring | do not edit `infini/`; no chrome; no EP-020 handshake |
| **D** | [STORY-IN-027](./stories/STORY-IN-027.md) | `infini/` `doc_change` applier | **done** |

Lane C is serial after EP-014. Do not pull residue. Do not start W10.

### Current-wave sub-agent roster

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| C | dev (now) | [STORY-EP-015](./stories/STORY-EP-015.md) | `epaper/` undo ring; host tests | in-review + QA handoff; no chrome; no infini |
| D | — | [STORY-IN-027](./stories/STORY-IN-027.md) | — | **done** — IN-028 stays draft |

## Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/ink-box chrome | Must | thick | [EP-012](./stories/STORY-EP-012.md) | **done** | W8 | spike closed | A |
| F-02 | epaper/device-document latency | Must | thick | — | **done** | W8 | — | B |
| F-03 | epaper/device-document tree | Must | thick | — | **done** | W9 | — | C |
| F-04 | epaper/device-document undo | Must | thick | — | **ready → in-progress** | **W9** | `/dev` (auto after QA walk) | C |
| F-05 | infini/tablet-sync applier | Must | thick | — | **done** | W9 | — | D |
| F-06 | epaper/ink-box enclose | Must | thick | — | draft | W10 | `/dev` | — |
| F-07 | epaper/ink-box membership | Must | thick | — | draft | W10 | `/dev` | — |
| F-08 | epaper/ink-box selection-create | Must | thick | EP-012 | **frozen draft** | W10 | later campaign (CHL-0010) | — |
| F-09 | epaper/ink-box manipulation | Must | thick | EP-012 | draft | W11 | `/dev` after EP-016; 28/56/96 locked | — |
| F-10 | epaper/device-document sync | Must | thick | — | draft | W12 | `/dev` | E |
| F-11 | infini/tablet-sync `doc_load` | Must | thick | — | draft | W12 | `/dev` | E ∥ |
| CHL-0008 | architecture rework | — | — | — | resolved / adopted | — | — | — |
| CHL-0009 | missing srs-logic.md | high | — | — | **resolved / adopted** | W8 | — | — |
| CHL-0010 | undo vs selection-create chrome | medium | — | — | **deferred** | W8 | — | EP-018 frozen |
| STORY-EP-006 / IN-019 | verify-fix | — | — | — | done (parked) | W7 void | — | — |
| EP-007…011, IN-020…026 | residue | — | — | — | blocked | — | — | — |

## Feature matrix

| Feature | Design / pipeline | Next |
|---|---|---|
| epaper/ink-box | EP-012 **done** `[UI-EP-02]`; 28/56/96 locked; CHL-0010 deferred | W10 enclose; W11 EP-019 |
| epaper/device-document | EP-014 **done**; EP-015 **ready** | `/qa` then `/dev` — ring, no chrome |
| epaper/tool-modes | EP-003/005 done; compose, don't redo | — |
| infini/tablet-sync | applier **done** | W12 IN-028 (`doc_load`; leftover `@future`) |

## Residual TBDs (do not block W9)

| Area | TBD |
|---|---|
| Pen miss-rate on 28/56 du handles | EP-019 QA — tune via CHL in du, never 8 CSS px / 0.35 |

## Backlog sink (do not schedule)

- epaper `[REQ-08]` node-manipulation — next campaign
- Desktop-side ink-box authoring (infini `[REQ-04]`) — until multi-directional sync
- On-device persistence, CRDT/OT, multi-document

## Verdict

**W9 continues.** EP-014 **done** (RM2 ingest p95=231 µs, arrival→flush 1392 µs, dropped=0).
EP-015 is **ready**: snapshot undo ring with **no on-panel chrome** (CHL-0010). Next `/qa`
(review `undo-ring.feature`, do not rewrite) then `/dev`. Do not start EP-016 / EP-018 /
EP-020 / IN-028. Do not pull residue.
