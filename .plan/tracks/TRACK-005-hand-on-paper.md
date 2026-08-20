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
  - STORY-EP-053
  - STORY-IN-036
  - STORY-EP-054
  - STORY-EP-055
  - STORY-IN-037
  - STORY-EP-056
  - STORY-EP-057
  - STORY-EP-058
cursor: "NOW EP-054 ∥ architect Device Settings (REQ-20); Dev blocked until /init"
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
| [IN-034](../iter-005/stories/STORY-IN-034.md) | design | P0 | historical Infini paint — superseded |
| [EP-056](../iter-005/stories/STORY-EP-056.md) | design | P0 | revise pen-button-map as epaper-device |
| [EP-052](../iter-005/stories/STORY-EP-052.md) | implement | P0 | barrel dispatch · depends EP-056 |
| [IN-035](../iter-005/stories/STORY-IN-035.md) | implement | — | **cancelled** — Infini persist retired |
| [EP-057](../iter-005/stories/STORY-EP-057.md) | implement | P0 | persist Device Settings on device |
| [EP-058](../iter-005/stories/STORY-EP-058.md) | implement | P0 | Settings page (Pen buttons) |

W0 bind **done** 2026-08-19 (`[SRS-EP-21]`…`[SRS-EP-48]`, `[SRS-IN-20]`…`[SRS-IN-25]`). EP-037 and IN-034 are **ready**. Other committed stories stay **draft** until their wave.

## Cursor

**Next:** Product Designer **EP-054** **∥** Solution Architect Device Settings rebind. Then Quality Assurance Engineer EP-038. Developer EP-038 / EP-039 / IN-033 waits on design + behavior-driven scenarios **and** `/init`.

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
| 2026-08-20 | Human: Device Settings saved on Epaper, not Infini, not document. PM REQ-20; Infini REQ-05 retired. CHL-0025 + GAP-01 adopted. Cursor → EP-054 ∥ architect rebind. IN-035 cancelled. |
