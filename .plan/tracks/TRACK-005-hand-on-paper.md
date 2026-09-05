---
id: TRACK-005
slug: hand-on-paper
kind: planned
status: done
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
  - STORY-EP-040
  - STORY-EP-041
  - STORY-EP-042
  - STORY-EP-043
  - STORY-EP-044
  - STORY-EP-045
  - STORY-EP-046
  - STORY-EP-047
  - STORY-IN-034
  - STORY-IN-035
  - STORY-EP-054
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
  - STORY-EP-074
  - STORY-EP-075
  - STORY-EP-076
  - STORY-EP-077
cursor: "done — closed 2026-09-05 because the stream was too large. Remainder → TRACK-007. Leftover: Path A EP-045/046 frozen; CHL-0027 Product Manager triage."
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
| [IN-033](../iter-005/stories/STORY-IN-033.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — Infini follow apply (F-IN-06) |
| [EP-040](../iter-005/stories/STORY-EP-040.md) | design | P0 | **cancelled** — icons only |
| [EP-041](../iter-005/stories/STORY-EP-041.md) | implement | P0 | **cancelled** — Path A |
| [EP-042](../iter-005/stories/STORY-EP-042.md) | implement | P0 | **cancelled** — Path B |
| [EP-043](../iter-005/stories/STORY-EP-043.md) | design | P0 | **cancelled** — chrome frozen in SRS |
| [EP-044](../iter-005/stories/STORY-EP-044.md) | implement | P0 | copy/cut/paste — **done** (human-verified 2026-09-04) |
| [EP-045](../iter-005/stories/STORY-EP-045.md) | design | P1 | **blocked** — Path A toolbar frozen (Infini later) |
| [EP-046](../iter-005/stories/STORY-EP-046.md) | implement | P1 | **blocked** — Path A apply frozen · depends EP-045 |
| [EP-047](../iter-005/stories/STORY-EP-047.md) | implement | P1 | Path B endpoint ink — **done** (human-verified 2026-09-05) |
| [EP-048](../iter-005/stories/STORY-EP-048.md) | design | P1 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — attachments (F-14) |
| [EP-049](../iter-005/stories/STORY-EP-049.md) | implement | P1 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — warp attachments (F-14) |
| [EP-050](../iter-005/stories/STORY-EP-050.md) | design | P2 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — manual create (F-17) |
| [EP-051](../iter-005/stories/STORY-EP-051.md) | implement | P2 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — insert nodes (F-17) |
| [IN-034](../iter-005/stories/STORY-IN-034.md) | design | P0 | historical Infini paint — superseded |
| [EP-056](../iter-005/stories/STORY-EP-056.md) | design | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — pen-button-map (F-18) |
| [EP-052](../iter-005/stories/STORY-EP-052.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — barrel dispatch (F-18) |
| [IN-035](../iter-005/stories/STORY-IN-035.md) | implement | — | **cancelled** — Infini persist retired |
| [EP-057](../iter-005/stories/STORY-EP-057.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — persist Device Settings (F-20) |
| [EP-058](../iter-005/stories/STORY-EP-058.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — Settings page (F-20) |
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
| [EP-070](../iter-005/stories/STORY-EP-070.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — residual pen-to-ink lag |
| [EP-071](../iter-005/stories/STORY-EP-071.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — selection settle probe (companion) |
| [EP-072](../iter-005/stories/STORY-EP-072.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — camera-change stress probe |
| [EP-073](../iter-005/stories/STORY-EP-073.md) | implement | P2 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — split clipboard clipops |
| [EP-074](../iter-005/stories/STORY-EP-074.md) | implement | P0 | Nested render + tap — **done** (human-verified 2026-09-05) |
| [EP-075](../iter-005/stories/STORY-EP-075.md) | implement | P0 | Nested enclose + flatten — **done** (human-verified 2026-09-05) · depends EP-074 |
| [EP-076](../iter-005/stories/STORY-EP-076.md) | implement | P1 | Move-commit reparent — **done** (human-verified 2026-09-05) · depends EP-074 |
| [EP-077](../iter-005/stories/STORY-EP-077.md) | implement | P0 | Clip nested content AABB — **done** (human-verified 2026-09-05) · depends EP-074 |
| [EP-078](../iter-005/stories/STORY-EP-078.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — spatial R-tree |
| [EP-079](../iter-005/stories/STORY-EP-079.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — migrate point-query callers |
| [EP-080](../iter-005/stories/STORY-EP-080.md) | implement | P0 | **carried → [TRACK-007](./TRACK-007-follow-through.md)** — migrate range 80% callers |

Follow design/implement that shipped on this track and then moved with F-19 / F-IN-06: [STORY-EP-053](../iter-005/stories/STORY-EP-053.md), [STORY-EP-055](../iter-005/stories/STORY-EP-055.md), [STORY-IN-036](../iter-005/stories/STORY-IN-036.md), [STORY-IN-037](../iter-005/stories/STORY-IN-037.md) — **carried → [TRACK-007](./TRACK-007-follow-through.md)** (residual human field test).

W0 bind **done** 2026-08-19. Delivered on this track: hand-touch, erase, clipboard product, Path B endpoint ink, nested ink-box, inverse-op undo. Remainder lives on TRACK-007.

## Cursor

**Done.** Closed 2026-09-05 because the stream was too large. Remainder → [TRACK-007](./TRACK-007-follow-through.md). Leftover here: Path A [STORY-EP-045](../iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](../iter-005/stories/STORY-EP-046.md) frozen; [CHL-0027](../iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md) Product Manager triage. Do **not** reopen this track. Do **not** reopen [TRACK-006](./TRACK-006-tool-system-refactor.md).

## Freeze note (closed 2026-09-05)

- In flight: none. Track **done**. Remainder on TRACK-007.
- Leftover here: Path A EP-045/046 **blocked**; [CHL-0027](../iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md) open; [CHL-0022](../iter-005/challenges/CHL-0022-shipped-no-device-pan.md) open.
- Resume: do **not** resume this track. Next work is TRACK-007. Do **not** start Path A toolbar.

## Execution board

Archive: [iter-005/execution-board.md](../iter-005/execution-board.md)

Canonical NOW board: [iter-005/execution-board-follow-through.md](../iter-005/execution-board-follow-through.md)

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
| 2026-09-05 | Human: more performance — logarithmic hit-test. Architect bound [SRS-EP-78](../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-78-log-hit-test) / [SRS-EP-79](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries) / [ADR-0040](../../../.docs/adr/ADR-0040-logarithmic-hit-test.md) (`proposed`). Sliced [STORY-EP-078](../iter-005/stories/STORY-EP-078.md)…[STORY-EP-080](../iter-005/stories/STORY-EP-080.md) **draft**, queued behind nested + field-latency cursor. |
| 2026-09-05 | Human **verified** nested ink-box [STORY-EP-074](../iter-005/stories/STORY-EP-074.md)…[STORY-EP-077](../iter-005/stories/STORY-EP-077.md). Log-hit-test `depends_on` unblocked. Cursor still field-latency EP-070…072. |
| 2026-09-05 | Human: close this track (too large). Remainder F-19, F-IN-06, F-18, F-20, F-14, F-17, EP-070, EP-071 (companion), EP-072, EP-078…080, EP-073 → [TRACK-007](./TRACK-007-follow-through.md). Track **done**. Path A leftover frozen. |
