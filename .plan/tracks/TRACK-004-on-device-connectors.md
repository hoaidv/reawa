---
id: TRACK-004
slug: on-device-connectors
kind: planned
status: active
iter: iter-004
goal: "On-device connectors: recognize hand-drawn ink between SmartGroups and keep it attached when a node moves"
scope: [epaper/connector-ink, epaper/tool-modes, epaper/ink-box, infini/vector-document]
stories: [STORY-EP-026, STORY-EP-027, STORY-EP-028, STORY-EP-029, STORY-EP-030, STORY-EP-031, STORY-IN-030]
cursor: "STORY-EP-026 ∥ STORY-EP-027 · /designer · execute-design-story"
paused_reason: ""
interrupts: []
---

# TRACK-004 — On-device connectors

## Goal

The creator draws a line between two ink-boxes and it stays a connection — their ink, attached,
alive when either box moves. Campaign ranked **above** REQ-08 / CHL-0011 / CHL-0012.

ADRs: [ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md) (Ink/Curve warp),
[ADR-0021](../../.docs/adr/ADR-0021-connector-toolchip.md) (3+2+Undo/Redo),
[ADR-0022](../../.docs/adr/ADR-0022-recognizer-dispatch.md) (one verdict per pen-up).
Product: [REQ-09](../../.docs/modules/epaper/prd.md#device-connectors), [REQ-03](../../.docs/modules/epaper/prd.md#tool-modes).
Brainstorm closed: [BS-0001](../iter-004/brainstorms/BS-0001-auto-connector-ink.md).
Spike (numeric reference only): [EXP-0002](../iter-004/explorations/EXP-0002-connector-ink-warp.md).

## Scope

- `epaper/connector-ink` — recognition, warp, blink + selection chrome
- `epaper/tool-modes` — ToolChip inventory (3 exclusive tools + 2 recognizer toggles)
- `epaper/ink-box` — closure-first dispatch / failed-enclose fall-through (D21)
- `infini/vector-document` — `create_connector` envelope + derived warp (viewer + persistence)

## Stories

| ID | Kind | Status | Notes |
|---|---|---|---|
| [STORY-EP-026](../iter-004/stories/STORY-EP-026.md) | design | **ready** | ToolChip 3+2+Undo/Redo · `toolchip-recognizers` |
| [STORY-EP-027](../iter-004/stories/STORY-EP-027.md) | design | **ready** | Connector blink + Ink/Curve chrome · `connector-chrome` |
| [STORY-EP-028](../iter-004/stories/STORY-EP-028.md) | implement | draft | ToolChip inventory — **depends EP-026** |
| [STORY-EP-029](../iter-004/stories/STORY-EP-029.md) | implement | draft | ADR-0022 dispatch — depends EP-028 |
| [STORY-EP-030](../iter-004/stories/STORY-EP-030.md) | implement | draft | UX1/UX2 recognition — depends EP-027, EP-029 |
| [STORY-EP-031](../iter-004/stories/STORY-EP-031.md) | implement | draft | Ink/Curve warp + live drag — depends EP-030 |
| [STORY-IN-030](../iter-004/stories/STORY-IN-030.md) | implement | draft | Infini envelope + derived warp — depends EP-030 |

## Cursor

**Next:** [STORY-EP-026](../iter-004/stories/STORY-EP-026.md) ∥ [STORY-EP-027](../iter-004/stories/STORY-EP-027.md) · persona `/designer` · skill `execute-design-story`

`/qa` may author BDD in parallel from SRS. `/dev` after BDD; UI implement waits on the matching design story `done`.

## Execution board

[iter-004/execution-board.md](../iter-004/execution-board.md)

## Freeze note (when paused)

- What was in flight:
- Open files / risks:
- Resume checklist:

## Log

| Date | Event |
|---|---|
| 2026-08-14 | Opened. BS-0001 concluded; REQ-09 + ADR-0020/21/22 adopted. Lock: horizontal · design-validated. Cursor → `/designer` EP-026 ∥ EP-027. |
| 2026-08-14 | Human flip: **vertical · verified · wip 2**. QA BDD unblocked; implement still `depends_on` design. |
