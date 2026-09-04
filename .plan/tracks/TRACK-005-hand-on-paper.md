---
id: TRACK-005
slug: hand-on-paper
kind: planned
status: active
iter: iter-005
goal: "Hand-on-paper wave: finger grammar (REQ-10), erase, clipboard, connector decorate, barrel buttons, manual create — not tables"
scope:
  - epaper/ink-box
  - epaper/tool-modes
  - epaper/connector-ink
  - epaper/region-sync
  - epaper/local-pen-ink
  - epaper/device-document
  - epaper/erase
  - infini/infinity-canvas
  - infini/tablet-sync
  - infini/vector-document
stories:
  - STORY-EP-037
  - STORY-EP-038
  - STORY-EP-039
  - STORY-IN-033
  - STORY-EP-040
  - STORY-EP-041
  - STORY-EP-042
  - STORY-EP-043
  - STORY-EP-044
  - STORY-EP-045
  - STORY-EP-046
  - STORY-EP-047
  - STORY-EP-048
  - STORY-EP-049
  - STORY-EP-050
  - STORY-EP-051
  - STORY-IN-034
  - STORY-EP-052
  - STORY-IN-035
  - STORY-EP-053
  - STORY-IN-036
  - STORY-EP-054
  - STORY-EP-055
  - STORY-IN-037
  - STORY-EP-056
  - STORY-EP-057
  - STORY-EP-058
  - STORY-EP-059
  - STORY-EP-060
  - STORY-EP-061
  - STORY-IN-038
  - STORY-EP-062
  - STORY-EP-063
  - STORY-EP-064
  - STORY-EP-065
  - STORY-EP-066
  - STORY-EP-067
  - STORY-EP-068
  - STORY-EP-069
  - STORY-EP-070
  - STORY-EP-071
  - STORY-EP-072
  - STORY-EP-073
cursor: "Field follow-ups EP-070…072 ready (ink lag, selection settle probe, camera stress). Clipboard EP-044 done (human-verified 2026-09-04). Path B endpoint ink EP-047 done (human-verified 2026-09-05). Path A EP-045/046 frozen. STORY-EP-073 later. STORY-EP-069 done."
paused_reason: ""
interrupts: []
---

# TRACK-005 — Hand-on-paper

## Goal

The creator uses **fingers** to pick, move, and navigate; **erase / copy / decorate connectors / barrel buttons / place objects** without leaving the tablet. **Not** table recognition ([REQ-15](../../.docs/modules/epaper/prd.md#table-recognition)). [REQ-16](../../.docs/modules/epaper/prd.md#device-pan-zoom) is retired into [REQ-10](../../.docs/modules/epaper/prd.md#hand-touch).

PRD: epaper 0.8.0-draft · infini 0.5.0-draft · [BS-0002](../iter-004/brainstorms/BS-0002-iter-005-feature-wave.md).

## Scope

- REQ-10 hand-touch (1-finger pick/move + 2-finger pan/zoom)
- REQ-11 erase · REQ-12 clipboard
- REQ-13 endpoint styles · REQ-14 mid-attachments
- REQ-17 manual create
- REQ-18 barrel-button catalogues + REQ-20 Device Settings (on-device persist). Infini REQ-05 persist/restore **retired**.

**Out:** REQ-15 tables · REQ-08 any-node · CHL-0011 / CHL-0012 · AI

## Stories

| ID | Kind | Pri | Notes |
|---|---|---|---|
| [EP-037](../iter-005/stories/STORY-EP-037.md) | design | P0 | hand-touch package |
| [EP-038](../iter-005/stories/STORY-EP-038.md) | implement | P0 | 1-finger · depends EP-037 |
| [EP-039](../iter-005/stories/STORY-EP-039.md) | implement | P0 | 2-finger pan · BRD-07 slice |
| [IN-033](../iter-005/stories/STORY-IN-033.md) | implement | P0 | Infini follow viewport · depends EP-039 |
| [EP-040](../iter-005/stories/STORY-EP-040.md) | design | P0 | **cancelled** — icons only |
| [EP-041](../iter-005/stories/STORY-EP-041.md) | implement | P0 | **cancelled** — Path A |
| [EP-042](../iter-005/stories/STORY-EP-042.md) | implement | P0 | **cancelled** — Path B |
| [EP-043](../iter-005/stories/STORY-EP-043.md) | design | P0 | **cancelled** — chrome frozen in SRS |
| [EP-044](../iter-005/stories/STORY-EP-044.md) | implement | P0 | copy/cut/paste — **done** (human-verified 2026-09-04) |
| [EP-045](../iter-005/stories/STORY-EP-045.md) | design | P1 | **blocked** — Path A toolbar frozen (Infini later) |
| [EP-046](../iter-005/stories/STORY-EP-046.md) | implement | P1 | **blocked** — Path A apply frozen · depends EP-045 |
| [EP-047](../iter-005/stories/STORY-EP-047.md) | implement | P1 | Path B endpoint ink — **done** (human-verified 2026-09-05) |
| [EP-048](../iter-005/stories/STORY-EP-048.md) | design | P1 | attachments |
| [EP-049](../iter-005/stories/STORY-EP-049.md) | implement | P1 | warp attachments |
| [EP-050](../iter-005/stories/STORY-EP-050.md) | design | P2 | manual create |
| [EP-051](../iter-005/stories/STORY-EP-051.md) | implement | P2 | insert nodes |
| [IN-034](../iter-005/stories/STORY-IN-034.md) | design | P0 | historical Infini paint — superseded |
| [EP-056](../iter-005/stories/STORY-EP-056.md) | design | P0 | revise pen-button-map as epaper-device |
| [EP-052](../iter-005/stories/STORY-EP-052.md) | implement | P0 | barrel dispatch · depends EP-056 |
| [IN-035](../iter-005/stories/STORY-IN-035.md) | implement | — | **cancelled** — Infini persist retired |
| [EP-057](../iter-005/stories/STORY-EP-057.md) | implement | P0 | persist Device Settings on device |
| [EP-058](../iter-005/stories/STORY-EP-058.md) | implement | P0 | Settings page (Pen buttons) |
| [EP-059](../iter-005/stories/STORY-EP-059.md) | implement | P0 | inverse ring + lastOpId — **done** |
| [EP-060](../iter-005/stories/STORY-EP-060.md) | implement | P0 | F20/F21 skip/no-op · depends EP-059 — **done** |
| [EP-061](../iter-005/stories/STORY-EP-061.md) | implement | P0 | device undo queue counterpart/compound — **done** |
| [IN-038](../iter-005/stories/STORY-IN-038.md) | implement | — | **cancelled** — tablet→desktop undo apply deferred |
| [EP-062](../iter-005/stories/STORY-EP-062.md) | implement | P0 | Eraser mode + ToolChip + barrel last-used — **done** |
| [EP-063](../iter-005/stories/STORY-EP-063.md) | implement | P0 | Clip engine + remnants + boundary polyline — **done** |
| [EP-064](../iter-005/stories/STORY-EP-064.md) | implement | P0 | Brush — **done** · depends EP-062, EP-063 |
| [EP-065](../iter-005/stories/STORY-EP-065.md) | implement | P0 | Area — **done** (human-verified 2026-08-31) · depends EP-062, EP-063, EP-067, EP-068 |
| [EP-066](../iter-005/stories/STORY-EP-066.md) | implement | P0 | Object 80% — **done** (human-verified 2026-08-31) · depends EP-062, EP-068 |
| [EP-067](../iter-005/stories/STORY-EP-067.md) | implement | P0 | Singleton generateNodeId — **done** (human-verified 2026-08-31) |
| [EP-068](../iter-005/stories/STORY-EP-068.md) | implement | P0 | Operations own overlay paint; ToolCanvasContext stays generic — **done** (human-verified 2026-08-31) |
| [EP-069](../iter-005/stories/STORY-EP-069.md) | implement | P0 | ToolContextImpl host ports and SelectionOverlay — **done** (human-verified 2026-08-31) |
| [EP-070](../iter-005/stories/STORY-EP-070.md) | implement | P0 | Residual pen-to-ink lag on moderately dense pages — **ready** |
| [EP-071](../iter-005/stories/STORY-EP-071.md) | implement | P0 | Instrument sel_rect / sel_freeform settle to knobs — **ready** |
| [EP-072](../iter-005/stories/STORY-EP-072.md) | implement | P0 | Camera-change stress probe on a full handwriting page — **ready** |
| [EP-073](../iter-005/stories/STORY-EP-073.md) | implement | P2 | Split clipboard clipops — **draft** later · depends EP-044 |

W0 bind **done** 2026-08-19 (`[SRS-EP-21]`…`[SRS-EP-48]`, `[SRS-IN-20]`…`[SRS-IN-25]`). Hand-touch and follow implement stories that shipped are **done**. Other committed stories stay **draft** until their wave. [STORY-IN-033](../iter-005/stories/STORY-IN-033.md) **done** 2026-08-20.

## Cursor

**NOW:** Field follow-ups [STORY-EP-070](../iter-005/stories/STORY-EP-070.md)…[STORY-EP-072](../iter-005/stories/STORY-EP-072.md) **ready** (human 2026-08-31 after LatestJob camera felt better). [STORY-EP-069](../iter-005/stories/STORY-EP-069.md) ToolContextImpl host ports / SelectionOverlay **done**. Erase [STORY-EP-062](../iter-005/stories/STORY-EP-062.md)…[STORY-EP-068](../iter-005/stories/STORY-EP-068.md) **done**. Clipboard [STORY-EP-044](../iter-005/stories/STORY-EP-044.md) **done**. Path B endpoint ink [STORY-EP-047](../iter-005/stories/STORY-EP-047.md) **done**. Path A [STORY-EP-045](../iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](../iter-005/stories/STORY-EP-046.md) frozen. [STORY-EP-073](../iter-005/stories/STORY-EP-073.md) later. Device Settings stay queued. Remaining Infini follow field test still outstanding.

Tool-system interrupt [TRACK-006](./TRACK-006-tool-system-refactor.md) is **done** (2026-08-27). Default pointer map is Primary=Pen, Secondary=Finger under [ADR-0033](../../../.docs/adr/ADR-0033-tool-abstraction.md).

## Freeze note (Path B endpoint ink done; Path A frozen; EP-073 later; field latency ready)

- In flight: none. Field follow-ups EP-070…072 **ready** (not started). [STORY-EP-069](../iter-005/stories/STORY-EP-069.md) ToolContextImpl/SelectionOverlay **done**, human verified 2026-08-31. Erase EP-062…068 **done**. EP-059…061 **done**, human verified undo/redo. Clipboard [STORY-EP-044](../iter-005/stories/STORY-EP-044.md) **done**, human verified 2026-09-04. Path B endpoint ink [STORY-EP-047](../iter-005/stories/STORY-EP-047.md) **done**, human verified 2026-09-05. Path A [STORY-EP-045](../iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](../iter-005/stories/STORY-EP-046.md) **blocked**. [STORY-EP-073](../iter-005/stories/STORY-EP-073.md) later. IN-038 **cancelled**.
- Open files / risks: [CHL-0027](../iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md) still open; no RM2 panel / no live TCP `:9877`; [CHL-0022](../iter-005/challenges/CHL-0022-shipped-no-device-pan.md) still open. Leftover snapshot wording in deprecated Infini [SRS-IN-12](../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) — do not implement.
- Resume: Human named ink lag / selection settle / camera stress for NOW. Clipboard product is done; Path B connector ends are done. Do **not** start EP-073 or Path A toolbar until the human picks them. Follow field-test notes still wanted. Do **not** reopen TRACK-006.

## Execution board

[iter-005/execution-board.md](../iter-005/execution-board.md)

## Log

| Date | Event |
|---|---|
| 2026-08-16 | Opened. Human: REQ-10→18 except REQ-15; lock TRACK-005 vertical. |
| 2026-08-19 | W0 architect bind landed (READY-WITH-CONCERNS). Stories retargeted. Cursor → W1 designer. |
| 2026-08-19 | W1 designer EP-037 ∥ IN-034 **done**. Cursor → W2 QA EP-038 after human visual. |
| 2026-08-20 | Human: independent cameras + optional one-way follow. BRD-07 lifted. ADR-0023 superseded by ADR-0029. Cursor → designer EP-053 ∥ IN-036. |
| 2026-08-20 | Follow design EP-053 ∥ IN-036 **done**. |
| 2026-08-20 | Human interrupt: pen-button-map is Epaper not Infini; Click/Hold-move catalogues shrunk (no temp freeform). Cursor → architect then EP-056. |
| 2026-08-20 | Architect ADR-0030; SRS-EP-52/53; SRS-IN-24 retired. Designer EP-056 **done** (UI-EP-08). |
| 2026-08-20 | Developer + Quality Assurance Engineer EP-055 ∥ IN-037 **done**. Cursor → IN-033 apply-depth. |
| 2026-08-20 | IN-033 **done** (host tests). Human: pause and deploy Infini + Epaper. Track **paused**. |
| 2026-08-20 | Human **approved hand-touch**. PRD 0.12.0-draft; architecture 20 mm / 178 du; UI-EP-06 HT + 20 mm. Track still **paused** for Infini follow score. |
| 2026-08-21 | Human: snapshot undo is **wrong**. Inverse-op + per-session stack + fail-safe no-op + no undo-through. [CHL-0026](../iter-005/challenges/CHL-0026-inverse-op-undo.md). [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) **proposed** (READY-WITH-CONCERNS). Cursor → Product Manager adopt. **No code.** |
| 2026-08-24 | Informal interrupt: tool-system refactor (later [TRACK-006](./TRACK-006-tool-system-refactor.md)). [CHL-0027](../iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md) opened. |
| 2026-08-27 | Human closed TRACK-006. This track is the **only active stream** again. Cursor unchanged: WAIT Product Manager adopt ADR-0032. |
| 2026-08-27 | Human **go** EP-060 + EP-061. [STORY-IN-038](../iter-005/stories/STORY-IN-038.md) **cancelled** — skip tablet→desktop undo apply until an independent sync algorithm. Cursor → Quality Assurance Engineer EP-060 + EP-061. |
| 2026-08-27 | EP-060 + EP-061 **done** (Quality Assurance Engineer PASS). Inverse-undo local complete. Cursor → wait human. W3 still frozen. |
| 2026-08-27 | Human **verified** device undo/redo complete (including resize after move). Cursor unchanged: wait human. W3 still frozen. |
| 2026-08-29 | [CHL-0028](../iter-005/challenges/CHL-0028-eraser-three-tools.md) **adopted**. Erase PRD + SRS + three icons. Implement stories EP-062…066 **draft**. Cursor → wait human review. Clipboard W3 still frozen. |
| 2026-08-30 | EP-067/068/065/066 implemented. Host tests green. Stories **in-review**. Cursor → wait human panel QA. Clipboard W3 still frozen. |
| 2026-08-31 | Human **verified** EP-067, EP-068, EP-065, EP-066. Erase implement **done**. Cursor → STORY-EP-069 in-progress. Clipboard W3 still frozen. |
| 2026-08-31 | Human **verified** [STORY-EP-069](../iter-005/stories/STORY-EP-069.md) (code already on device). Cursor → WAIT human next pick. Clipboard W3 still frozen. |
| 2026-08-31 | Human: LatestJob camera **better**. Filed [STORY-EP-070](../iter-005/stories/STORY-EP-070.md) residual pen-to-ink, [STORY-EP-071](../iter-005/stories/STORY-EP-071.md) selection settle probe, [STORY-EP-072](../iter-005/stories/STORY-EP-072.md) camera stress. Clipboard still frozen. |
| 2026-09-04 | Human **verified** [STORY-EP-044](../iter-005/stories/STORY-EP-044.md) clipboard complete. Filed [STORY-EP-073](../iter-005/stories/STORY-EP-073.md) clipops split — later, not NOW. |
| 2026-09-05 | Human **verified** Path B connector endpoint styles. [STORY-EP-047](../iter-005/stories/STORY-EP-047.md) **done**. Path A [STORY-EP-045](../iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](../iter-005/stories/STORY-EP-046.md) stay frozen. |
