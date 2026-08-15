---
title: Execution board — on-device connectors
iter: iter-004
track: TRACK-004
owner: sm
date: 2026-08-15
lock: vertical · verified · wip 2 · epaper/{connector-ink, tool-modes, ink-box} + infini/vector-document
verdict: "NOW — /qa verify EP-028 + IN-031"
wave: W2-verify
---

# Execution board — on-device connectors

**Canonical board** for [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) in this iter.
MASTER + tracks stay the **guiding spine**; this file is the **operational map**.

---

## Summary (as of 2026-08-15)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 2 | EP-026, EP-027 |
| Wave **NOW** | W2 verify | **`/qa` EP-028 + IN-031** |
| Implement gated | 4 | EP-029 → EP-030 → EP-031 ∥ IN-030 |

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
personas: /qa (NOW verify EP-028 + IN-031)
forbidden: REQ-08; CHL-0011; CHL-0012; desktop connector authoring; IN-006 DocChrome
NOW: W2 — /qa verify STORY-EP-028 + STORY-IN-031
cursor: /qa verify-story both lanes
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | — | PM REQ-09 + REQ-03; Architect ADR-0020/21/22 |
| **W1** | **done** | **∥ yes** | Design ToolChip + connector chrome |
| **W2** | **NOW** | **∥ yes** | `/qa` verify ToolChip (EP-028) **∥** hide Infini ToolStrip (IN-031) |
| **W3** | next | EP-030 then EP-031 ∥ IN-030 | Recognize, warp, Infini mirror |
| **W-last** | later | serial | Human verify + `/pm` `validated_by` |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **D** | [STORY-EP-028](./stories/STORY-EP-028.md) | `epaper/` (ToolChip QML) | do not edit `infini/` |
| **E** | [STORY-IN-031](./stories/STORY-IN-031.md) | `infini/src/canvas/` ToolStrip + overlay | do not edit `epaper/` |

Lanes **∥**. Shared `.docs/` only if stitching; do not parallel-edit the same file.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/tool-modes | Must | thick | EP-026 | **done** | W2 | `/dev` EP-029 | — |
| F-02 | epaper/connector-ink (chrome) | Must | thick | EP-027 | **done** | W1 | wait W3 | — |
| F-03 | epaper/ink-box (dispatch) | Must | thick | — | ready | W2→W3 | `/dev` after EP-028 | — |
| F-04 | epaper/connector-ink (recognize + warp) | Must | thick | EP-027 | ready | W3 | `/dev` after EP-029 | — |
| F-05 | infini/vector-document (mirror) | Must | thick | n/a | ready | W3 | `/dev` after EP-030 | — |
| F-06 | infini hide ToolStrip | Must | REQ-04 | n/a (remove) | **done** | W2 | — | **E** |
| CHORE-1 | EXP-0002 Initiative 2 guard corpus | Ship | — | — | open | ∥ | `/qa` explore | — |
| CHORE-2 | PM `validated_by` | — | — | campaign verify | later | W-last | `/pm` | serial |

### Current-wave sub-agent roster (spawn when wave = NOW)

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| D | qa | [STORY-EP-028](./stories/STORY-EP-028.md) | verify `epaper/` ToolChip BDD | pass → `done`; fail → DEF + blocked |
| E | qa | [STORY-IN-031](./stories/STORY-IN-031.md) | verify hide-toolbar BDD | pass → `done`; fail → DEF + blocked |

### Residual TBDs (do not block current wave unless marked)

| Area | TBD |
|---|---|
| Blink waveform / duration | Designer picked in EP-027 (~250 ms Mono) |
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
| tool-modes | **done** (EP-028) | `/dev` EP-029 |
| Infini hide toolbar | **done** (IN-031) | — |
| connector-ink | design **done** | W3 `/dev` |
| ink-box dispatch | BDD ready | `/dev` EP-029 after EP-028 |
| infini vector-document | BDD ready | W3 `/dev` IN-030 |

---

## Verdict

**READY** — W2 verified (human + host tests). EP-028 and IN-031 **done**. Next: **`/dev` EP-029**. Do not start REQ-08.
