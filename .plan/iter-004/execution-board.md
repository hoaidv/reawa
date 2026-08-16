---
title: Execution board — on-device connectors
iter: iter-004
track: TRACK-004
owner: sm
date: 2026-08-16
lock: vertical · verified · wip 2 · epaper/{connector-ink, tool-modes, ink-box} + infini/vector-document
verdict: "NOW — EP-034/036 done; connectivity bugs fixed; next EP-035"
wave: ep-036-def
---

# Execution board — on-device connectors

**Canonical board** for [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) in this iter.
MASTER + tracks stay the **guiding spine**; this file is the **operational map**.

---

## Summary (as of 2026-08-16)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 2 | EP-026, EP-027 |
| Wave **NOW** | IN-032 | **`in-review`** live drag + no demo |
| Implement gated | soak | EP-034 / EP-036 **HOLD** |

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
personas: /dev EP-034; human IN-030 live review
forbidden: REQ-08; CHL-0011; CHL-0012; desktop connector authoring; IN-006 DocChrome; iter-005 slice
NOW: IN-032 live drag + no demo
cursor: /qa IN-032; EP-034/036 HOLD
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | — | PM REQ-09 + REQ-03; Architect ADR-0020/21/22 |
| **W1** | **done** | **∥ yes** | Design ToolChip + connector chrome |
| **W2** | **done** | **∥ yes** | `/qa` verify ToolChip (EP-028) **∥** hide Infini ToolStrip (IN-031) |
| **hotfix** | **done** | — | P0 [STORY-EP-033](./stories/STORY-EP-033.md) origin/stale — human verified |
| **ops** | **NOW** | **∥ IN-030 review** | P1 [STORY-EP-034](./stories/STORY-EP-034.md) StrokeSync TCP (ping-alive class) |
| **measure** | queued | after WIP | P1 [STORY-EP-035](./stories/STORY-EP-035.md) enclose A/L log only |
| **W3** | **done** (host) | live extra | Infini mirror IN-030 — human `create_connector` still wanted |
| **W-last** | later | serial | Human verify + `/pm` `validated_by` |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **U** | [STORY-EP-034](./stories/STORY-EP-034.md) | StrokeSync/debug TCP keepalive, Infini listen sockets, deploy ssh keepalives | no enclose; no ingest |
| **R** | [STORY-IN-030](./stories/STORY-IN-030.md) live review | none (story `done`) | needs `:9877` up — blocked until EP-034 reconnects |

Observed 2026-08-16: ping `10.11.99.1` 13/13; epaper/Infini **not** connected → **StrokeSync TCP**, not gadget death. Do not unplug. EP-035 stays queued.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/tool-modes | Must | thick | EP-026 | **done** | W2 | — | — |
| F-02 | epaper/connector-ink (chrome) | Must | thick | EP-027 | **done** | W1 | — | — |
| F-03 | epaper/ink-box (dispatch) | Must | thick | — | **done** | W2 | human verified | — |
| F-04 | epaper/connector-ink (recognize + warp) | Must | thick | EP-027 | **done** | W3 | human verified EP-031 | — |
| F-05 | infini/vector-document (mirror) | Must | thick | n/a | **done** (host) | W3 | human live `create_connector` | **R** |
| F-06 | infini hide ToolStrip | Must | REQ-04 | n/a (remove) | **done** | W2 | — | **E** |
| CHORE-1 | EXP-0002 Initiative 2 guard corpus | Ship | — | — | open | ∥ | `/qa` explore | — |
| CHORE-2 | PM `validated_by` | — | — | campaign verify | later | W-last | `/pm` | serial |
| BUG-01 | origin/stale pen-down diagonal | P0 | SRS-EP-01 | — | **done** | hotfix | — | **H** |
| BUG-02 | USB Ethernet ping-alive Infini-down | P1 | SRS-EP-08 | — | **in-review** | **NOW** | `/qa` EP-034 | **U** |
| CHORE-3 | Enclose A/L handwriting vs boundary (measure only) | P1 | SRS-EP-10 | — | ready | queued | `/dev` EP-035 | — |
| CHORE-4 | IN-030 live create_connector | P1 | SRS-IN-09 | — | open | **NOW ∥** | human (needs sync) | **R** |
| BUG-03 | Infini demo figures mix with RM ink | P1 | SRS-IN-07 | — | ready | queued | `/dev` IN-032 | — |
| BUG-04 | USB plugged, ping timeout / no 10.11.99.1 | P1 | SRS-EP-08 | — | ready | queued | `/dev` EP-036 after EP-034 | — |

### Current-wave sub-agent roster (spawn when wave = NOW)

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| U | dev | [STORY-EP-034](./stories/STORY-EP-034.md) | StrokeSync/Infini TCP keepalive + 2s retry | Infini shows RM connected without USB replug |
| R | human | [STORY-IN-030](./stories/STORY-IN-030.md) | live `create_connector` on wire | extra, not a host-BDD reopen |

### Residual TBDs (do not block current wave unless marked)

| Area | TBD |
|---|---|
| Blink waveform / duration | Enclose: ~250 ms Mono. Membership: last-join highlight (UI-EP-06); no blink |
| Device chrome state machine | STORY-EP-032 draft — `/architect` later; not NOW |
| Connector select + hand-touch | PM → architect 2026-08-15; not NOW until SRS/stories exist |
| Pen-down origin diagonal | STORY-EP-033 — **done** |
| USB gadget / SSH keepalive | STORY-EP-034 — **in-review** `/qa`; ping-alive Infini-down |
| Infini demo mix | STORY-IN-032 — queued |
| USB plugged, ping dead | STORY-EP-036 — queued; restore without unplug |
| Handwriting vs boundary A/L | STORY-EP-035 — queued |
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

---

## Feature matrix (quick view)

| Feature | Design / pipeline | Next |
|---|---|---|
| tool-modes | **done** (EP-028) | — |
| Infini hide toolbar | **done** (IN-031) | — |
| connector-ink | design **done**; recognize + warp **done** | human verified EP-031 |
| local-pen-ink (ingest) | P0 EP-033 **done** | — |
| ink-box dispatch | **done** (EP-029, human verified) | — |
| infini vector-document | **done** host (IN-030) | live `create_connector` after EP-034 reconnects |

---

## Verdict

**NOW** — [STORY-EP-035](./stories/STORY-EP-035.md) enclose A/L (queued/`ready`). EP-034 / EP-036 **done**. DEF-0001/0002/0003 **fixed**. IN-032 **done**.
