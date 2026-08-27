---
id: TRACK-006
slug: tool-system-refactor
kind: expedite
status: done
iter: iter-005
goal: "Close the tool-system refactor interrupt: Tablet/Tool split, ADR-0033 Router/Mode/Operation, dissolve host bags, pointer roles — then resume TRACK-005"
scope:
  - epaper/tool-modes
  - epaper/ink-box
stories: []
cursor: "done — human closed 2026-08-27; leftover DeviceMap UI / Mouse handler parked"
paused_reason: ""
interrupts: [TRACK-005]
---

# TRACK-006 — Tool system refactor

## Goal

Give Epaper a registerable tool system so new gestures are new files, not new `onPointerStart` branches. This stream was an **informal interrupt** of [TRACK-005](./TRACK-005-hand-on-paper.md) (Hand-on-paper) from 2026-08-24 through 2026-08-27. Human 2026-08-27: **close it** and return to TRACK-005.

Normative lock: [ADR-0033](../../.docs/adr/ADR-0033-tool-abstraction.md) (Tool system abstraction — Router, Mode, Operation, Modifiers) **accepted**. Working memory: [plan_epaper-tool-system-refactor.md](../../.docs/memory/plan_epaper-tool-system-refactor.md), [plan_dissolve_host_bags.md](../../.docs/memory/plan_dissolve_host_bags.md), [plan_toolaction_context_ui.md](../../.docs/memory/plan_toolaction_context_ui.md), [usecase-epaper-tool-system.md](../../.docs/memory/refactoring-skills/usecase-epaper-tool-system.md).

## Scope

- `epaper/drawing/tabletcanvasitem.*` — document surface (ink, rasterize, sync)
- `epaper/drawing/toolcanvasitem.*` — Qt entry + hub register/forward
- `epaper/drawing/tools/` — InputHub, modes, operations, modifiers, contexts, actions, ui

**Out of this close (parked, not next work):** invert DeviceMap user interface; Mouse `DragHandler`; further taxonomy polish unless a TRACK-005 story needs it. EraserMode body belongs to TRACK-005 wave W3 ([REQ-11](../../.docs/modules/epaper/prd.md#erase)), not this stream.

## Stories

None sliced. Architecture enabler interrupt — no `STORY-*` ids.

## Waves that landed (git 2026-08-24…27)

| Wave | What | Notes |
|---|---|---|
| A | Peel domain out of TabletCanvas | Coordinate types, sessions, machines — still one canvas |
| B | Tablet / Tool split | `CanvasSession`; Surface API / Interaction API; `ToolCanvas.qml` |
| C | ADR-0033 taxonomy | Mode objects, Operations, HandTouch match→lock (`6dcab20`…`4683639`) |
| D | Dissolve host bags | Operations own bodies; delete Finger/Manip/Stroke hosts and intent-appliers |
| E | ToolAction + leftover sessions | Click actions, selection chrome, registered interventions |
| F | Cleanup + pointer roles | Primary/Secondary `DeviceMap`; `InkMode`; `SecondaryDeviceModifier` (`dc379d6`). Finger lasso/marquee disabled (`dd57741`) |

## Cursor

**Done.** Human closed 2026-08-27. Do **not** continue this stream. Next work is TRACK-005.

## Execution board

[iter-005/execution-board.md](../iter-005/execution-board.md) — TRACK-005 board. This interrupt is a **done** wave row there, not a second board.

## Freeze note (closed)

- In flight at close: none. Working tree clean on `main`. Last commits: `dc379d6` (Primary/Secondary devices), `dd57741` (disable finger lasso/marquee).
- Open files / risks: ToolCanvasItem is router host (~266 lines). Default map still Primary=Pen, Secondary=Finger. Pen-near cancel is still physical stylus (known leftover after a DeviceMap swap).
- Parked: DeviceMap invert user interface; Mouse handler; EraserMode (TRACK-005 W3).
- Resume this stream: **do not.** Further tool-system work only if a TRACK-005 story requires it.

## Log

| Date | Event |
|---|---|
| 2026-08-24 | Informal interrupt of TRACK-005. God-class grouping on TabletCanvasItem. |
| 2026-08-25 | Tablet / Tool split landed. |
| 2026-08-26 | ADR-0033 accepted. Taxonomy phases 0–6. |
| 2026-08-27 | Dissolve bags + ToolAction leftovers + pointer roles. Human: **close**. Track materialized as done. |
