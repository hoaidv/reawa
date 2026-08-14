---
title: Execution board — on-device connectors
iter: iter-004
track: TRACK-004
owner: sm
date: 2026-08-14
lock: vertical · verified · wip 2 · epaper/{connector-ink, tool-modes, ink-box} + infini/vector-document
verdict: "READY — W1 design done; QA BDD then EP-028"
wave: W2-bdd
---

# Execution board — on-device connectors

**Canonical board** for [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) in this iter.
MASTER + tracks stay the **guiding spine**; this file is the **operational map**.

---

## Summary (as of 2026-08-14)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 2 | EP-026, EP-027 |
| Wave **NOW** | BDD | `/qa` then EP-028 |
| API-only / no UI paint | 0 | — |
| Implement gated | 5 | `depends_on` design + BDD — stop line no longer freezes them |

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
personas: /designer (NOW) · /qa (BDD ∥) · /dev (after BDD + design depends_on)
forbidden: REQ-08; CHL-0011; CHL-0012; desktop connector authoring; implement UI before its design story is done
NOW: W2 — /qa BDD then STORY-EP-028
cursor: /qa write-bdd then /dev implement-story
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | — | PM REQ-09 + REQ-03; Architect ADR-0020/21/22 + SRS-EP-17…20 |
| **W1** | **done** | **∥ yes** | Design ToolChip + connector chrome |
| **W2** | **NOW** | serial | `/qa` BDD; then ToolChip implement (EP-028) |
| **W3** | next | EP-030 then EP-031 ∥ IN-030 | Recognize, warp, Infini mirror |
| **W-last** | later | serial | Human verify + `/pm` `validated_by` |

### Parallelism rules (current wave)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-026](./stories/STORY-EP-026.md) | `design/toolchip-recognizers/` | do not edit `connector-chrome/` |
| **B** | [STORY-EP-027](./stories/STORY-EP-027.md) | `design/connector-chrome/` | do not edit `toolchip-recognizers/` |

Lanes **∥**. Shared `.docs/DESIGN.md` / tokens: stitch with designer; do not parallel-edit the same token file.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/tool-modes | Must | thick | EP-026 | **done** | W1 | `/qa` | — |
| F-02 | epaper/connector-ink (chrome) | Must | thick | EP-027 | **done** | W1 | `/qa` | — |
| F-03 | epaper/ink-box (dispatch) | Must | thick | — (logic) | draft | W2 | `/qa` BDD now; `/dev` after EP-028 | — |
| F-04 | epaper/connector-ink (recognize + warp) | Must | thick | EP-027 | draft | W3 | `/qa` BDD now (logic); UI waits EP-027 | — |
| F-05 | infini/vector-document | Must | thick | n/a | draft | W3 | `/qa` BDD now; `/dev` after EP-030 | — |
| CHORE-1 | EXP-0002 Initiative 2 guard corpus | Ship | — | — | open | ∥ | `/qa` explore — ship gate | — |
| CHORE-2 | PM `validated_by` | — | — | campaign verify | later | W-last | `/pm` after human confirm | serial |

### Current-wave sub-agent roster (spawn when wave = NOW)

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| A | designer | [STORY-EP-026](./stories/STORY-EP-026.md) | `design/toolchip-recognizers/` | `ui-spec-gate` + story `done` |
| B | designer | [STORY-EP-027](./stories/STORY-EP-027.md) | `design/connector-chrome/` | `ui-spec-gate` + story `done` |
| C | qa | EP-029 / EP-031 / IN-030 BDD | `epaper/features/**/bdd/`, `infini/features/vector-document/` | READY-FOR-DEV; do not edit design packages |

### Residual TBDs (do not block current wave unless marked)

| Area | TBD |
|---|---|
| Blink waveform / duration | Designer picks in EP-027; not a brainstorm |
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

---

## Feature matrix (quick view)

| Feature | Design / pipeline | Next |
|---|---|---|
| tool-modes | in-progress (EP-026 ready) | `/designer` W1-A |
| connector-ink | in-progress (EP-027 ready) | `/designer` W1-B |
| ink-box dispatch | BDD-ready (logic) | `/qa` then W2 `/dev` |
| infini vector-document | BDD-ready (logic) | `/qa` then W3 `/dev` |

---

## Verdict

**READY** — W1 design is **done** ([UI-EP-04](./design/toolchip-recognizers/), [UI-EP-05](./design/connector-chrome/)). Next: **`/qa`** then **`/dev` EP-028**. Open [CHL-0019](./challenges/CHL-0019-toolchip-tile-size.md) for `/pm` (64 px tiles vs SRS 32). Do not start REQ-08.
