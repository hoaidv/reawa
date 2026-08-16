---
title: Execution board — on-device connectors
iter: iter-004
track: TRACK-004
owner: sm
date: 2026-08-16
lock: vertical · verified · wip 1 · epaper/{connector-ink, tool-modes, ink-box} + infini/vector-document
verdict: "FROZEN — TRACK-004 human verified 2026-08-16; EP-035 carried; EP-036 cancelled"
wave: closed
---

# Execution board — on-device connectors

**Canonical board** for [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) in this iter.
MASTER + tracks stay the **guiding spine**; this file is the **operational map**.

---

## Summary (as of 2026-08-16)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 2 | EP-026, EP-027 |
| Wave **NOW** | EP-035 | `/dev` — measure enclose A/L; no verdict change |
| Implement **done** this wave | EP-034, EP-036, IN-030, IN-032 | PM gated 2026-08-16 |

Design front target: **2/2** in-scope UI packages.

---

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 1
out_of_scope: backlog
modules: epaper, infini
features: epaper/connector-ink; epaper/tool-modes; epaper/ink-box; infini/vector-document
personas: /dev EP-035
forbidden: REQ-08; CHL-0011; CHL-0012; desktop connector authoring; IN-006 DocChrome; iter-005 slice
NOW: STORY-EP-035 enclose A/L measure
cursor: /dev EP-035
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | — | PM REQ-09 + REQ-03; Architect ADR-0020/21/22 |
| **W1** | **done** | **∥ yes** | Design ToolChip + connector chrome |
| **W2** | **done** | **∥ yes** | `/qa` verify ToolChip (EP-028) **∥** hide Infini ToolStrip (IN-031) |
| **hotfix** | **done** | — | P0 [STORY-EP-033](./stories/STORY-EP-033.md) origin/stale |
| **ops** | **done** | — | P1 EP-034 StrokeSync TCP + EP-036 gadget restore |
| **W3** | **done** | — | Infini mirror IN-030 + IN-032 demo mix |
| **measure** | **NOW** | serial | P1 [STORY-EP-035](./stories/STORY-EP-035.md) enclose A/L log only |
| **W-last** | later | serial | Human verify + `/pm` `validated_by` |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **M** | [STORY-EP-035](./stories/STORY-EP-035.md) | enclose/dispatch `[recog]` logs, host analog, notes table | do not change enclose verdict |

WIP 1 — no second implement lane.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/tool-modes | Must | thick | EP-026 | **done** | W2 | — | — |
| F-02 | epaper/connector-ink (chrome) | Must | thick | EP-027 | **done** | W1 | — | — |
| F-03 | epaper/ink-box (dispatch) | Must | thick | — | **done** | W2 | — | — |
| F-04 | epaper/connector-ink (recognize + warp) | Must | thick | EP-027 | **done** | W3 | — | — |
| F-05 | infini/vector-document (mirror) | Must | thick | n/a | **done** | W3 | — | — |
| F-06 | infini hide ToolStrip | Must | REQ-04 | n/a | **done** | W2 | — | — |
| CHORE-1 | EXP-0002 Initiative 2 guard corpus | Ship | — | — | open | ∥ | `/qa` explore | — |
| CHORE-2 | PM `validated_by` | — | — | campaign verify | later | W-last | `/pm` | serial |
| BUG-01 | origin/stale pen-down diagonal | P0 | SRS-EP-01 | — | **done** | hotfix | — | — |
| BUG-02 | USB Ethernet ping-alive Infini-down | P1 | SRS-EP-08 | — | **done** | ops | — | — |
| CHORE-3 | Enclose A/L handwriting vs boundary | P1 | SRS-EP-10 | — | **ready** | **NOW** | `/dev` EP-035 | **M** |
| CHORE-4 | IN-030 live create_connector | P1 | SRS-IN-09 | — | **done** | W3 | — | — |
| BUG-03 | Infini demo figures mix with RM ink | P1 | SRS-IN-07 | — | **done** | W3 | — | — |
| BUG-04 | USB plugged, ping timeout / no 10.11.99.1 | P1 | SRS-EP-08 | — | **done** | ops | — | — |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| M | dev | [STORY-EP-035](./stories/STORY-EP-035.md) | `[recog]` A/L fields + host analog + notes scaffold | logs exist; enclose verdict unchanged |

### Residual TBDs (do not block current wave unless marked)

| Area | TBD |
|---|---|
| Device chrome state machine | STORY-EP-032 draft — `/architect` later; not NOW |
| Connector select + hand-touch | PM → architect 2026-08-15; not NOW until SRS/stories exist |
| Inflection cutoff on a real corpus | EXP-0002 Initiative 2 / QA — ship gate |

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
- iter-005 slice (wait retro-gate)

---

## Feature matrix (quick view)

| Feature | Design / pipeline | Next |
|---|---|---|
| tool-modes | **done** (EP-028) | — |
| Infini hide toolbar | **done** (IN-031) | — |
| connector-ink | **done** (EP-031) | — |
| local-pen-ink (ingest) | **done** (EP-033) | — |
| ink-box dispatch | **done** (EP-029) | EP-035 measure A/L |
| infini vector-document | **done** (IN-030, IN-032) | — |
| USB / StrokeSync | **done** (EP-034, EP-036) | — |

---

## Verdict

**FROZEN.** Human verified TRACK-004. [STORY-EP-035](./stories/STORY-EP-035.md) carried to iter-005. [STORY-EP-036](./stories/STORY-EP-036.md) **cancelled**.
