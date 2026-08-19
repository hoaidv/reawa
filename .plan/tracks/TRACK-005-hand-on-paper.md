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
cursor: "W2 /qa BDD EP-038 then /dev EP-038 — human visual on W1 packages first"
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
- REQ-18 + infini REQ-05 barrel-button catalogues

**Out:** REQ-15 tables · REQ-08 any-node · CHL-0011 / CHL-0012 · AI

## Stories

| ID | Kind | Pri | Notes |
|---|---|---|---|
| [EP-037](../iter-005/stories/STORY-EP-037.md) | design | P0 | hand-touch package |
| [EP-038](../iter-005/stories/STORY-EP-038.md) | implement | P0 | 1-finger · depends EP-037 |
| [EP-039](../iter-005/stories/STORY-EP-039.md) | implement | P0 | 2-finger pan · BRD-07 slice |
| [IN-033](../iter-005/stories/STORY-IN-033.md) | implement | P0 | Infini follow viewport · depends EP-039 |
| [EP-040](../iter-005/stories/STORY-EP-040.md) | design | P0 | erase chrome |
| [EP-041](../iter-005/stories/STORY-EP-041.md) | implement | P0 | eraser nib |
| [EP-042](../iter-005/stories/STORY-EP-042.md) | implement | P0 | selection-erase |
| [EP-043](../iter-005/stories/STORY-EP-043.md) | design | P0 | clipboard chrome |
| [EP-044](../iter-005/stories/STORY-EP-044.md) | implement | P0 | copy/cut/paste |
| [EP-045](../iter-005/stories/STORY-EP-045.md) | design | P1 | endpoint toolbar |
| [EP-046](../iter-005/stories/STORY-EP-046.md) | implement | P1 | end styles |
| [EP-047](../iter-005/stories/STORY-EP-047.md) | implement | P1 | endpoint ink |
| [EP-048](../iter-005/stories/STORY-EP-048.md) | design | P1 | attachments |
| [EP-049](../iter-005/stories/STORY-EP-049.md) | implement | P1 | warp attachments |
| [EP-050](../iter-005/stories/STORY-EP-050.md) | design | P2 | manual create |
| [EP-051](../iter-005/stories/STORY-EP-051.md) | implement | P2 | insert nodes |
| [IN-034](../iter-005/stories/STORY-IN-034.md) | design | P0 | Infini button map |
| [EP-052](../iter-005/stories/STORY-EP-052.md) | implement | P0 | barrel dispatch · depends IN-034 |
| [IN-035](../iter-005/stories/STORY-IN-035.md) | implement | P0 | publish map |

W0 bind **done** 2026-08-19 (`[SRS-EP-21]`…`[SRS-EP-48]`, `[SRS-IN-20]`…`[SRS-IN-25]`). EP-037 and IN-034 are **ready**. Other committed stories stay **draft** until their wave.

## Cursor

**Next:** Human visual check of W1 packages, then Quality Assurance Engineer behavior-driven scenarios for **EP-038**, then Developer.

## Execution board

[iter-005/execution-board.md](../iter-005/execution-board.md)

## Log

| Date | Event |
|---|---|
| 2026-08-16 | Opened. Human: REQ-10→18 except REQ-15; lock TRACK-005 vertical. |
| 2026-08-19 | W0 architect bind landed (READY-WITH-CONCERNS). Stories retargeted. Cursor → W1 designer. |
| 2026-08-19 | W1 designer EP-037 ∥ IN-034 **done**. Cursor → W2 QA EP-038 after human visual. |
