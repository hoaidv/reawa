---
id: STORY-EP-071
title: Instrument sel_rect and sel_freeform settle to knobs
kind: implement
parent_srs: [SRS-EP-12, SRS-EP-04]
parent_req: [REQ-06, REQ-03]
status: ready
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given sel_rect or sel_freeform, When SelectionPhase::Selecting is in flight, Then the live marquee or lasso overlay stays the existing Operation paintOverlay path and this story does not make that path slower."
  - "Given pointer-up that commits a selection, When the settled dotted AABB with knobs appears, Then a dedicated always-on log line records pointer-up → chrome-visible with named stages (hit/commit, overlay show, paintSettled, damage/update, waveform) and milliseconds per stage."
  - "Given the same moderately dense page as STORY-EP-070 (~4 sentences + ink-boxes), When the creator lassos or marquees then lifts, Then the log is enough to say whether settle lag is hit-test, SelectionOverlay paint, ToolCanvas Mono refresh, or something else — not a guess."
  - "Given the probe, When EPAPER_SEL_SETTLE=0 (or the chosen env), Then it is off; default on device is on, same pattern as ink-path / rasterize probes."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-071 — Instrument sel_rect and sel_freeform settle to knobs

Field 2026-08-31 on the same moderately dense page as the residual ink lag: **Selecting** (live
freeform polyline / rect) is still **smooth**. **Settle** is slower than expected: from pointer-up
until the settled selection rect with knobs is shown (human: eight knobs on panel).

This story is **instrument first**. Do not redesign chrome. Do not merge
`SelectionOverlay` into `ToolCanvasItem` ([STORY-EP-069](./STORY-EP-069.md) is done).

Canonical: [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-12-selection-chrome)
(overlay); [SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-04)
(routing). Live gesture paint stays on `LassoOperation` / `MarqueeOperation`; settled AABB/knobs
stay on `SelectionOverlay`.

Human is Quality Assurance Engineer on device. No design package. No behavior-driven ceremony.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — (can run beside [STORY-EP-070](./STORY-EP-070.md) / [STORY-EP-072](./STORY-EP-072.md); different write set: overlay settle vs ink vs camera job) |

## Split today (do not invert)

| Phase | Owns | Field |
|---|---|---|
| `SelectionPhase::Selecting` | Operation `paintOverlay` | Smooth — out of scope to “fix” |
| Pointer-up → settled knobs | commit + `SelectionOverlay::paintSettled` + ToolCanvas show/damage | **Slow — instrument this** |

## Done when

- Device log: one line (or a tight span chain) per settle, stages + ms
- Human can tail the log while reproducing the lag
- No clipboard / Device Settings work in this story
