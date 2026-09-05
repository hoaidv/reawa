---
id: TRACK-007
slug: follow-through
kind: planned
status: active
iter: iter-005
goal: "Hand-on-paper remainder: viewport-follow field score, barrel + Device Settings, attachments, manual create, field latency, logarithmic hit-test, clipboard clipops split"
scope:
  - epaper/ink-box
  - epaper/tool-modes
  - epaper/connector-ink
  - epaper/region-sync
  - epaper/local-pen-ink
  - epaper/device-document
  - infini/infinity-canvas
  - infini/tablet-sync
  - infini/vector-document
stories:
  - STORY-EP-053
  - STORY-EP-055
  - STORY-IN-036
  - STORY-IN-037
  - STORY-IN-033
  - STORY-EP-056
  - STORY-EP-052
  - STORY-EP-057
  - STORY-EP-058
  - STORY-EP-070
  - STORY-EP-071
  - STORY-EP-072
  - STORY-EP-078
  - STORY-EP-079
  - STORY-EP-080
  - STORY-EP-073
  - STORY-EP-048
  - STORY-EP-049
  - STORY-EP-050
  - STORY-EP-051
cursor: "WAIT human pick first wave. Closed TRACK-005 was too large; nothing is NOW until the human names a wave."
paused_reason: ""
interrupts: []
---

# TRACK-007 — Hand-on-paper follow-through

## Goal

Finish the remainder of the hand-on-paper campaign after [TRACK-005](./TRACK-005-hand-on-paper.md) closed 2026-09-05. Same iteration ([iter-005](../iter-005/iter.md)); **not** a new iteration.

Landed on TRACK-005 and **not** this stream: hand-touch, erase, clipboard product, Path B endpoint ink, nested ink-box, inverse-op undo, tool-system interrupt.

## Scope

Human 2026-09-05 move list, plus companion [STORY-EP-071](../iter-005/stories/STORY-EP-071.md) (Instrument sel_rect and sel_freeform settle to knobs) so a closed track does not own a **ready** priority-zero story.

| Label | Product | Stories |
|---|---|---|
| F-19 | [REQ-19](../../.docs/modules/epaper/prd.md#viewport-follow) viewport-follow Infini | [STORY-EP-053](../iter-005/stories/STORY-EP-053.md) / [STORY-EP-055](../iter-005/stories/STORY-EP-055.md) **done**; residual **human field test** |
| F-IN-06 | Infini [REQ-06](../../.docs/modules/infini/prd.md#viewport-follow) viewport-follow Epaper | [STORY-IN-036](../iter-005/stories/STORY-IN-036.md) / [STORY-IN-037](../iter-005/stories/STORY-IN-037.md) / [STORY-IN-033](../iter-005/stories/STORY-IN-033.md) **done**; residual **human field test** |
| F-18 | [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) barrel accelerators | [STORY-EP-056](../iter-005/stories/STORY-EP-056.md) **done**; [STORY-EP-052](../iter-005/stories/STORY-EP-052.md) **draft** |
| F-20 | [REQ-20](../../.docs/modules/epaper/prd.md#device-settings) Device Settings | [STORY-EP-057](../iter-005/stories/STORY-EP-057.md) / [STORY-EP-058](../iter-005/stories/STORY-EP-058.md) **draft** |
| F-14 | [REQ-14](../../.docs/modules/epaper/prd.md#connector-attachments) connector mid-attachments | [STORY-EP-048](../iter-005/stories/STORY-EP-048.md) / [STORY-EP-049](../iter-005/stories/STORY-EP-049.md) **draft** |
| F-17 | [REQ-17](../../.docs/modules/epaper/prd.md#manual-create) manual create | [STORY-EP-050](../iter-005/stories/STORY-EP-050.md) / [STORY-EP-051](../iter-005/stories/STORY-EP-051.md) **draft** |
| — | Field latency | [STORY-EP-070](../iter-005/stories/STORY-EP-070.md) / [STORY-EP-071](../iter-005/stories/STORY-EP-071.md) / [STORY-EP-072](../iter-005/stories/STORY-EP-072.md) **ready** |
| — | Logarithmic hit-test | [STORY-EP-078](../iter-005/stories/STORY-EP-078.md)…[STORY-EP-080](../iter-005/stories/STORY-EP-080.md) **draft**; [ADR-0040](../../.docs/adr/ADR-0040-logarithmic-hit-test.md) `proposed` |
| — | Clipboard clipops split | [STORY-EP-073](../iter-005/stories/STORY-EP-073.md) **draft** later |

**Out of this track:** Path A toolbar [STORY-EP-045](../iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](../iter-005/stories/STORY-EP-046.md) stay frozen leftovers on TRACK-005. [CHL-0027](../iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md) stays a TRACK-005 leftover for Product Manager triage. Do **not** reopen [TRACK-006](./TRACK-006-tool-system-refactor.md). Do **not** open iter-006 until Product Manager retro-gate for iter-005.

## Stories

| ID | Kind | Pri | Status | Notes |
|---|---|---|---|---|
| [EP-053](../iter-005/stories/STORY-EP-053.md) | design | P0 | **done** | viewport-follow Epaper [UI-EP-07](../iter-005/design/viewport-follow-epaper/) |
| [EP-055](../iter-005/stories/STORY-EP-055.md) | implement | P0 | **done** | Epaper follow toggle · residual human field test |
| [IN-036](../iter-005/stories/STORY-IN-036.md) | design | P0 | **done** | viewport-follow Infini [UI-IN-04](../iter-005/design/viewport-follow-infini/) |
| [IN-037](../iter-005/stories/STORY-IN-037.md) | implement | P0 | **done** | Infini follow toggle · residual human field test |
| [IN-033](../iter-005/stories/STORY-IN-033.md) | implement | P0 | **done** | Infini apply while following · residual human field test |
| [EP-056](../iter-005/stories/STORY-EP-056.md) | design | P0 | **done** | pen-button map as Epaper [UI-EP-08](../iter-005/design/pen-button-map/) |
| [EP-052](../iter-005/stories/STORY-EP-052.md) | implement | P0 | **draft** | barrel click vs hold-move · depends EP-056 |
| [EP-057](../iter-005/stories/STORY-EP-057.md) | implement | P0 | **draft** | persist Device Settings · depends EP-056 |
| [EP-058](../iter-005/stories/STORY-EP-058.md) | implement | P0 | **draft** | Settings page Pen buttons · depends EP-056 |
| [EP-070](../iter-005/stories/STORY-EP-070.md) | implement | P0 | **ready** | residual pen-to-ink lag |
| [EP-071](../iter-005/stories/STORY-EP-071.md) | implement | P0 | **ready** | selection settle probe (companion; not on the human move list) |
| [EP-072](../iter-005/stories/STORY-EP-072.md) | implement | P0 | **ready** | camera-change stress probe |
| [EP-078](../iter-005/stories/STORY-EP-078.md) | implement | P0 | **draft** | spatial R-tree · depends EP-074 **done** |
| [EP-079](../iter-005/stories/STORY-EP-079.md) | implement | P0 | **draft** | migrate point-query callers · depends EP-078 |
| [EP-080](../iter-005/stories/STORY-EP-080.md) | implement | P0 | **draft** | migrate range 80% callers · depends EP-078, EP-076 **done** |
| [EP-073](../iter-005/stories/STORY-EP-073.md) | implement | P2 | **draft** | split clipboard clipops · later |
| [EP-048](../iter-005/stories/STORY-EP-048.md) | design | P1 | **draft** | attachments |
| [EP-049](../iter-005/stories/STORY-EP-049.md) | implement | P1 | **draft** | attachments warp · depends EP-048 |
| [EP-050](../iter-005/stories/STORY-EP-050.md) | design | P2 | **draft** | manual create |
| [EP-051](../iter-005/stories/STORY-EP-051.md) | implement | P2 | **draft** | manual insert · depends EP-050 |

## Cursor

**WAIT.** Human pick first wave. Do **not** start field-latency, logarithmic hit-test, Device Settings, attachments, or manual create until named.

When a wave starts: work-in-progress 2. Do **not** run [STORY-EP-070](../iter-005/stories/STORY-EP-070.md) and [STORY-EP-072](../iter-005/stories/STORY-EP-072.md) together (same `tabletcanvasitem.cpp`). [ADR-0040](../../.docs/adr/ADR-0040-logarithmic-hit-test.md) is still `proposed` — Product Manager accept before logarithmic-hit-test implement.

## Execution board

[iter-005/execution-board-follow-through.md](../iter-005/execution-board-follow-through.md)

TRACK-005 archive: [iter-005/execution-board.md](../iter-005/execution-board.md)

## Freeze note

- In flight: none. Track just opened; cursor is wait-for-human.
- Open files / risks: no RM2 panel / no live TCP `:9877` for remaining follow field test; [ADR-0040](../../.docs/adr/ADR-0040-logarithmic-hit-test.md) `proposed`.
- Resume: Human names a wave from the board. Do **not** reopen TRACK-005 or TRACK-006. Do **not** start Path A toolbar.

## Log

| Date | Event |
|---|---|
| 2026-09-05 | Opened. Human closed TRACK-005 (too large) and moved F-19, F-IN-06, F-18, F-20, F-14, F-17, EP-070, EP-072, EP-078…080, EP-073 here. Scrum Master also moved EP-071 with the field-latency pair. Same iteration. |
