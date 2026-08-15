---
title: Execution board — on-device connectors
iter: iter-004
track: TRACK-004
owner: sm
date: 2026-08-15
lock: vertical · verified · wip 2 · epaper/{connector-ink, tool-modes, ink-box} + infini/vector-document
verdict: "NOW — /dev EP-030 (human verified through EP-029); EP-034 USB P1 queued"
wave: W3-recognize
---

# Execution board — on-device connectors

**Canonical board** for [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) in this iter.
MASTER + tracks stay the **guiding spine**; this file is the **operational map**.

---

## Summary (as of 2026-08-15)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 2 | EP-026, EP-027 |
| Wave **NOW** | W3 | **`/dev` EP-030** |
| Implement gated | 3 | EP-030 → EP-031 ∥ IN-030 |

Design front target: **2/2** in-scope UI packages.

---

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
out_of_scope: backlog
modules: epaper, infini
features: epaper/connector-ink; epaper/tool-modes; epaper/ink-box; infini/vector-document
personas: /dev (NOW EP-030)
forbidden: REQ-08; CHL-0011; CHL-0012; desktop connector authoring; IN-006 DocChrome
NOW: W3 — /dev STORY-EP-030
cursor: /dev EP-030
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | — | PM REQ-09 + REQ-03; Architect ADR-0020/21/22 |
| **W1** | **done** | **∥ yes** | Design ToolChip + connector chrome |
| **W2** | **done** | **∥ yes** | `/qa` verify ToolChip (EP-028) **∥** hide Infini ToolStrip (IN-031) |
| **hotfix** | queued | serial vs EP-030 | P0 [STORY-EP-033](./stories/STORY-EP-033.md) origin/stale pen-down |
| **ops** | queued | no conflict vs EP-030 | P1 [STORY-EP-034](./stories/STORY-EP-034.md) USB stay-up / keepalives |
| **W3** | **NOW** | EP-030 then EP-031 ∥ IN-030 | Recognize, warp, Infini mirror |
| **W-last** | later | serial | Human verify + `/pm` `validated_by` |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **C** | [STORY-EP-030](./stories/STORY-EP-030.md) | `epaper/` connector recognize | **do not ∥ EP-033** (`tabletcanvasitem.cpp`) |

Serial vs EP-033. No Infini writes until IN-030.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/tool-modes | Must | thick | EP-026 | **done** | W2 | `/dev` EP-029 | — |
| F-02 | epaper/connector-ink (chrome) | Must | thick | EP-027 | **done** | W1 | wait W3 | — |
| F-03 | epaper/ink-box (dispatch) | Must | thick | — | **done** | W2 | human verified | — |
| F-04 | epaper/connector-ink (recognize + warp) | Must | thick | EP-027 | ready | W3 | `/dev` EP-030 | — |
| F-05 | infini/vector-document (mirror) | Must | thick | n/a | ready | W3 | `/dev` after EP-030 | — |
| F-06 | infini hide ToolStrip | Must | REQ-04 | n/a (remove) | **done** | W2 | — | **E** |
| CHORE-1 | EXP-0002 Initiative 2 guard corpus | Ship | — | — | open | ∥ | `/qa` explore | — |
| CHORE-2 | PM `validated_by` | — | — | campaign verify | later | W-last | `/pm` | serial |
| BUG-01 | origin/stale pen-down diagonal | P0 | SRS-EP-01 | — | ready | queued | `/dev` EP-033 after EP-030 | — |
| BUG-02 | USB Ethernet unreachable until replug | P1 | SRS-EP-08 | — | ready | queued | `/dev` EP-034 after EP-030 | — |

### Current-wave sub-agent roster (spawn when wave = NOW)

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| C | dev | [STORY-EP-030](./stories/STORY-EP-030.md) | connector recognition + blink | in-review → `/qa` |

### Residual TBDs (do not block current wave unless marked)

| Area | TBD |
|---|---|
| Blink waveform / duration | Enclose: ~250 ms Mono. Membership: last-join highlight (UI-EP-06); no blink |
| Device chrome state machine | STORY-EP-032 draft — `/architect` later; not NOW |
| Pen-down origin diagonal | STORY-EP-033 — queued after EP-030 |
| USB gadget / SSH keepalive | STORY-EP-034 — queued; ping-dead vs ping-alive |
| Inflection cutoff on a real corpus | EXP-0002 Initiative 2 / QA — ship gate |
| Live-drag panel rate on RM2 | Measure in EP-031, not lock |

### PM parking lot (do not forget)

| Id | Topic | Status | Parallel? |
|---|---|---|---|
| PL-01 | REQ-08 any-node manip | deferred | — |
| PL-02 | CHL-0011 nested enclose | deferred | — |
| PL-03 | CHL-0012 FREE_FORM / align-content | deferred | — |

### Backlog sink (do not schedule)

- Desktop connector authoring
- Arrowheads / dash / double-line / squared routing
- Physics rope (EH2)
- Connectors to non-SmartGroup kinds
- STORY-IN-006 DocChrome

---

## Feature matrix (quick view)

| Feature | Design / pipeline | Next |
|---|---|---|
| tool-modes | **done** (EP-028) | — |
| Infini hide toolbar | **done** (IN-031) | — |
| connector-ink | design **done** | **W3 `/dev` EP-030** |
| local-pen-ink (ingest) | P0 EP-033 queued | after EP-030 |
| ink-box dispatch | **done** (EP-029, human verified) | — |
| infini vector-document | BDD ready | W3 `/dev` IN-030 after EP-030 |

---

## Verdict

**NOW** — **`/dev` EP-030**. EP-029 human-verified. EP-033 origin-diagonal and EP-034 USB stay-up stay queued. Do not start REQ-08.
