---
title: Execution board — on-device connectors
iter: iter-004
track: TRACK-004
owner: sm
date: 2026-08-14
lock: horizontal · design-validated · epaper/{connector-ink, tool-modes, ink-box} + infini/vector-document
verdict: "READY-WITH-CONCERNS — design NOW; implement frozen; guard corpus is a ship gate"
wave: W1-design
---

# Execution board — on-device connectors

**Canonical board** for [TRACK-004](../tracks/TRACK-004-on-device-connectors.md) in this iter.
MASTER + tracks stay the **guiding spine**; this file is the **operational map**.

---

## Summary (as of 2026-08-14)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 0 | EP-026, EP-027 not painted |
| Wave **NOW** (paint) | 2 | EP-026 ∥ EP-027 |
| API-only / no UI paint | 0 | — |
| Implement freeze | all 5 | Until `validated_by` + stop_line flip |

Design front target: **2/2** in-scope UI packages.

---

## Lock (copy into every sub-agent brief)

```
direction: horizontal
stop_line: design-validated
autonomy: bounded
out_of_scope: backlog
modules: epaper, infini
features: epaper/connector-ink; epaper/tool-modes; epaper/ink-box; infini/vector-document
personas: /designer (NOW) · /pm (gate) · /architect (SRS already in)
forbidden: /dev implement; /qa story BDD until stop_line flips; REQ-08; CHL-0011; CHL-0012; desktop connector authoring
NOW: W1 — STORY-EP-026 ∥ STORY-EP-027
cursor: /designer execute-design-story
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | — | PM REQ-09 + REQ-03; Architect ADR-0020/21/22 + SRS-EP-17…20 |
| **W1** | **NOW** | **∥ yes** | Design ToolChip + connector chrome |
| **W2** | frozen | serial | Implement ToolChip (EP-028) + dispatch (EP-029) after design-validated |
| **W3** | frozen | serial | Recognize (EP-030) then warp (EP-031) ∥ Infini mirror (IN-030 after EP-030) |
| **W-last** | next | serial | `/pm` `validated_by` then flip stop_line |
| **Frozen** | until flip | — | all `kind: implement` |

### Parallelism rules (current wave)

| Lane | Story | Package / writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-026](./stories/STORY-EP-026.md) | `design/toolchip-recognizers/` | do not edit `connector-chrome/` |
| **B** | [STORY-EP-027](./stories/STORY-EP-027.md) | `design/connector-chrome/` | do not edit `toolchip-recognizers/` |

Lanes **∥**. Shared `.docs/DESIGN.md` / tokens: stitch with designer; do not parallel-edit the same token file.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group |
|---|---|---|---|---|---|---|---|---|
| F-01 | epaper/tool-modes | Must | thick | EP-026 | ready | W1 | `/designer` | A |
| F-02 | epaper/connector-ink (chrome) | Must | thick | EP-027 | ready | W1 | `/designer` | B |
| F-03 | epaper/ink-box (dispatch) | Must | thick | — (logic) | frozen | W2 | `/dev` after EP-028 | — |
| F-04 | epaper/connector-ink (recognize + warp) | Must | thick | EP-027 | frozen | W3 | `/dev` | — |
| F-05 | infini/vector-document | Must | thick | n/a | frozen | W3 | `/dev` | — |
| CHORE-1 | EXP-0002 Initiative 2 guard corpus | Ship | — | — | open | ∥ design | `/qa` explore — **not** story BDD | — |
| CHORE-2 | PM `validated_by` | — | — | EP-026+027 done | ready | W-last | `/pm` | serial |

### Current-wave sub-agent roster (spawn when wave = NOW)

| Lane | Agent role | Story | Package / writes | Done when |
|---|---|---|---|---|
| A | designer | [STORY-EP-026](./stories/STORY-EP-026.md) | `design/toolchip-recognizers/` | `ui-spec-gate` + story `done` |
| B | designer | [STORY-EP-027](./stories/STORY-EP-027.md) | `design/connector-chrome/` | `ui-spec-gate` + story `done` |

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
| ink-box dispatch | API-only this wave | freeze until W2 |
| infini vector-document | API-only this wave | freeze until W3 |

---

## Verdict

**READY-WITH-CONCERNS** — campaign is locked and TRACK-004 is open. Next serial step is **`/designer`** on EP-026 ∥ EP-027. Concerns: (1) EXP-0002 guard corpus still open — blocks **ship**, not this stop line; (2) implement UI must not go `ready` until both design stories are `done`. Do not start REQ-08.
