---
title: Execution board — hand-on-paper follow-through
iter: iter-005
track: TRACK-007
owner: sm
date: 2026-09-05
lock: vertical · verified · wip 2
wave: WAIT
verdict: "TRACK-005 closed 2026-09-05 (too large). TRACK-007 remainder is queued. No NOW wave until the human names one. Nested ink-box EP-074…077 done on TRACK-005. ADR-0040 still proposed."
---

# Execution board — hand-on-paper follow-through

**Canonical board** for [TRACK-007](../tracks/TRACK-007-follow-through.md). Track **active**. Same iteration as the closed [TRACK-005](../tracks/TRACK-005-hand-on-paper.md) archive board: [execution-board.md](./execution-board.md). Tool-system interrupt [TRACK-006](../tracks/TRACK-006-tool-system-refactor.md) **done** 2026-08-27 — do **not** reopen.

---

## Summary (as of 2026-09-05)

| Band | Count | Meaning |
|---|---|---|
| Wave **NOW** | 0 | Wait for the human to name a first wave |
| Follow residual | 2 chores | F-19 and F-IN-06 implement **done**; remaining **human field test** |
| Ready (not started) | 3 | Field latency [STORY-EP-070](./stories/STORY-EP-070.md)…[STORY-EP-072](./stories/STORY-EP-072.md) |
| Draft implement | 6 | Barrel [STORY-EP-052](./stories/STORY-EP-052.md); Device Settings [STORY-EP-057](./stories/STORY-EP-057.md) / [STORY-EP-058](./stories/STORY-EP-058.md); logarithmic hit-test [STORY-EP-078](./stories/STORY-EP-078.md)…[STORY-EP-080](./stories/STORY-EP-080.md); clipboard split [STORY-EP-073](./stories/STORY-EP-073.md) |
| Draft design | 2 | Attachments [STORY-EP-048](./stories/STORY-EP-048.md); manual create [STORY-EP-050](./stories/STORY-EP-050.md) |

Delivered on TRACK-005 (not this board’s NOW): hand-touch, erase, clipboard product, Path B endpoint ink, nested ink-box, inverse-op undo.

---

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
out_of_scope: backlog
modules: epaper, infini
features: epaper/ink-box; epaper/tool-modes; epaper/connector-ink; epaper/region-sync; epaper/local-pen-ink; epaper/device-document; epaper/erase; infini/infinity-canvas; infini/tablet-sync; infini/vector-document
personas: product-manager, architect, designer, quality-assurance-engineer, developer, human
forbidden: REQ-15 tables; REQ-08; CHL-0012; EP-032; AI; last-writer ADR-0023; TRACK-005 reopen; TRACK-006 reopen; DeviceMap invert UI; Mouse DragHandler; STORY-IN-038; infini undo apply; tablet-to-desktop undo sync; Path A toolbar EP-045/046
NOW: none — wait human pick
cursor: human names first TRACK-007 wave; do not auto-start EP-070 and EP-072 together; ADR-0040 still proposed
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **WAIT** | **NOW** | — | Human names the first wave. Nothing in progress. |
| **W-follow-field** | queued | serial | Human field test for F-19 and F-IN-06 |
| **W-field-latency** | queued (ready) | **∥** (WIP 2; not A+C) | [STORY-EP-070](./stories/STORY-EP-070.md) residual ink lag · [STORY-EP-071](./stories/STORY-EP-071.md) selection settle · [STORY-EP-072](./stories/STORY-EP-072.md) camera stress |
| **W-log-hit-test** | queued (draft) | serial then ∥ migrates | [STORY-EP-078](./stories/STORY-EP-078.md) R-tree · [STORY-EP-079](./stories/STORY-EP-079.md) point callers · [STORY-EP-080](./stories/STORY-EP-080.md) range 80% callers. [ADR-0040](../../.docs/adr/ADR-0040-logarithmic-hit-test.md) proposed. |
| **W-barrel-settings** | queued (draft) | serial | Quality Assurance Engineer then [STORY-EP-052](./stories/STORY-EP-052.md) / [STORY-EP-058](./stories/STORY-EP-058.md) / [STORY-EP-057](./stories/STORY-EP-057.md) |
| **W-attachments** | queued | serial | Designer [STORY-EP-048](./stories/STORY-EP-048.md) then [STORY-EP-049](./stories/STORY-EP-049.md) |
| **W-manual-create** | queued | serial | Designer [STORY-EP-050](./stories/STORY-EP-050.md) then [STORY-EP-051](./stories/STORY-EP-051.md) |
| **W-clipboard-refactor** | later (draft) | | [STORY-EP-073](./stories/STORY-EP-073.md) split clipops |
| **Frozen** | — | | Path A [STORY-EP-045](./stories/STORY-EP-045.md) / [STORY-EP-046](./stories/STORY-EP-046.md) leftover on TRACK-005 |

### Parallelism rules (current wave)

No NOW lanes. When field-latency starts:

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-070](./stories/STORY-EP-070.md) | ink-path / TabletCanvas sample path | same `tabletcanvasitem.cpp` as C |
| **B** | [STORY-EP-071](./stories/STORY-EP-071.md) | SelectionOverlay / select Operations settle | overlay files; can run beside A if A stays in ink-path |
| **C** | [STORY-EP-072](./stories/STORY-EP-072.md) | rasterize probe / camera job log | `tabletcanvasitem.cpp` + `rasterize_probe` — conflicts with A |

Work-in-progress 2. Do **not** run A and C in parallel.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group | Progress Detail |
|---|---|---|---|---|---|---|---|---|---|
| F-19 | REQ-19 viewport-follow Infini | Must | SRS bound | [STORY-EP-053](./stories/STORY-EP-053.md) **done** | EP-055 **done** | W-follow-field | human field test | — | Toggle verified on host. Residual: no device/Qt build. Carried from TRACK-005. |
| F-IN-06 | Infini REQ-06 viewport-follow Epaper | Must | SRS bound | [STORY-IN-036](./stories/STORY-IN-036.md) **done** | IN-037 + IN-033 **done** | W-follow-field | human field test | — | Toggle + apply-while-following host-verified. Residual: no live TCP `:9877` / no RM2. Carried from TRACK-005. |
| F-18 | REQ-18 barrel accelerators | Must | PRD; [UI-EP-08](./design/pen-button-map/) | [STORY-EP-056](./stories/STORY-EP-056.md) **done** | design done; EP-052 **draft** | W-barrel-settings | Quality Assurance Engineer then [STORY-EP-052](./stories/STORY-EP-052.md) | — | Catalogues/dispatch. Settings shell is REQ-20. |
| F-20 | REQ-20 Device Settings | Must | PRD; [ADR-0031](../../.docs/adr/ADR-0031-device-settings-persist-on-epaper.md) | EP-056 painted | EP-057 / EP-058 **draft** | W-barrel-settings | Quality Assurance Engineer then EP-058 / EP-057 | — | On-device persist. |
| F-14 | REQ-14 attachments | Should | — | [STORY-EP-048](./stories/STORY-EP-048.md) **draft** | queued | W-attachments | designer | — | Needs design package before implement. |
| F-17 | REQ-17 manual create | Should | — | [STORY-EP-050](./stories/STORY-EP-050.md) **draft** | queued | W-manual-create | designer | — | Needs design package before implement. |
| STORY-EP-070 | Residual pen-to-ink lag on moderately dense pages | P0 | SRS-EP-01 / EP-03 | — | **ready** | W-field-latency | developer (when human picks) | A | Do not FullClear ink ([CHL-0029](./challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)). Conflicts with EP-072 on `tabletcanvasitem.cpp`. |
| STORY-EP-071 | Instrument sel_rect and sel_freeform settle to knobs | P0 | SRS-EP-12 / EP-04 | — | **ready** | W-field-latency | developer (when human picks) | B | Companion moved with field-latency even though not on the human move list. |
| STORY-EP-072 | Camera-change stress probe on a full handwriting page | P0 | SRS-EP-03 / EP-24 | — | **ready** | W-field-latency | developer (when human picks) | C | Conflicts with EP-070 on `tabletcanvasitem.cpp`. |
| STORY-EP-078 | Spatial R-tree and named geometry queries | P0 | SRS-EP-79 / EP-78; [ADR-0040](../../.docs/adr/ADR-0040-logarithmic-hit-test.md) proposed | none | **draft** | W-log-hit-test | developer (after Product Manager accept ADR-0040 and human pick) | — | Nested tap EP-074 **done**. Owns the index. |
| STORY-EP-079 | Migrate point-query callers to geometry index | P0 | SRS-EP-79 / EP-77 / EP-11 / EP-21 | — | **draft** | W-log-hit-test | developer (after EP-078) | — | Tap / move / select / finger / paste-tap. |
| STORY-EP-080 | Migrate range 80-percent callers to geometry index | P0 | SRS-EP-79 / EP-11 / EP-10 / EP-75 / EP-58 | — | **draft** | W-log-hit-test | developer (after EP-078) | — | Marquee, freeform, enclose, draw-into, move-reparent; object-erase cull only. |
| STORY-EP-073 | Split clipboard clipops into document helpers and actions | P2 | SRS-EP-31 / EP-07 | — | **draft** later | W-clipboard-refactor | developer (when human picks) | — | Structure only. Product clipboard [STORY-EP-044](./stories/STORY-EP-044.md) already **done** on TRACK-005. |
| STORY-EP-049 | Mid-attachments follow connector warp | P1 | SRS-EP-38 / EP-40 | EP-048 | **draft** | W-attachments | developer (after EP-048) | — | Depends on design. |
| STORY-EP-051 | Manual insert frame connector primitive | P2 | SRS-EP-45 / EP-46 / EP-48 | EP-050 | **draft** | W-manual-create | developer (after EP-050) | — | Depends on design. |
| CHORE-follow | Viewport-follow hardware score | Must | ADR-0029 | EP-053 / IN-036 **done** | open | W-follow-field | human | — | Host tests passed. Needs Infini + Epaper on device. |

### Current-wave sub-agent roster

Empty. Do **not** spawn until the human names a wave.

### Backlog sink

| Item | Why |
|---|---|
| REQ-15 tables | human excluded |
| REQ-08 / CHL-0012 | parked |
| Path A EP-045 / EP-046 | leftover on TRACK-005; Infini / web-desktop later |
| TRACK-005 reopen | closed 2026-09-05 |
| TRACK-006 reopen | closed 2026-08-27 |
| DeviceMap invert user interface | TRACK-006 leftover |
| Mouse DragHandler | TRACK-006 leftover |
| Infini undo apply | [STORY-IN-038](./stories/STORY-IN-038.md) **cancelled** |
| CHL-0027 palm travel | TRACK-005 leftover; Product Manager triage |

---

## Verdict

[TRACK-005](../tracks/TRACK-005-hand-on-paper.md) **done** 2026-09-05. This board waits. Pick one wave: follow field test, field latency, logarithmic hit-test (after Architecture Decision Record 0040 accept), barrel and Device Settings, attachments design, or manual-create design. Do **not** start Path A toolbar. Do **not** open a new iteration.
