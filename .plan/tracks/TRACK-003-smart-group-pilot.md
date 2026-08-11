---
id: TRACK-003
slug: smart-group-pilot
kind: planned
status: active
iter: iter-003
cursor: "TRACK-003 UI+tools shipped — await human verify / next track"
goal: "Ink-box pilot: infini REQ-04 + epaper REQ-03 tool modes"
owner: sm
---

# TRACK-003 — Smart Group pilot

## Goal

Deliver the ink-box pilot across both ends: Infini
[REQ-04](../../.docs/modules/infini/prd.md#smart-group) and Epaper
[REQ-03](../../.docs/modules/epaper/prd.md#tool-modes). ADRs:
[ADR-0011](../../.docs/adr/ADR-0011-smart-group.md),
[ADR-0013](../../.docs/adr/ADR-0013-ink-box-tool-modes.md).

## Stories

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-IN-015](../iter-003/stories/STORY-IN-015.md) | implement | **done** | selection + UV |
| [STORY-IN-012](../iter-003/stories/STORY-IN-012.md) | implement | **done** | ink ingestion prerequisite |
| [STORY-EP-004](../iter-003/stories/STORY-EP-004.md) | implement | **done** | RM2 touch spike |
| [STORY-IN-013](../iter-003/stories/STORY-IN-013.md) | design | **done** | ink-box-ui |
| [STORY-IN-014](../iter-003/stories/STORY-IN-014.md) | implement | **done** | undo |
| [STORY-IN-010](../iter-003/stories/STORY-IN-010.md) | implement | **done** | enclose (rewritten) |
| [STORY-IN-016](../iter-003/stories/STORY-IN-016.md) | implement | **done** | membership |
| [STORY-IN-017](../iter-003/stories/STORY-IN-017.md) | implement | **done** | surround create + ToolStrip |
| [STORY-EP-003](../iter-003/stories/STORY-EP-003.md) | design | **done** | epaper strip |
| [STORY-IN-018](../iter-003/stories/STORY-IN-018.md) | implement | **done** | transport |
| [STORY-EP-005](../iter-003/stories/STORY-EP-005.md) | implement | **done** | device ToolChip + intent |

## Board

[execution-board](../iter-003/execution-board.md)

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after iter-002 retro-gate |
| 2026-08-11 | PM ink-box UX; scope + epaper; WIP 2 |
| 2026-08-11 | Architect ADR-0013 + SRS; then confirm IN-15/16 + UV |
| 2026-08-11 | SM sliced 11 stories; cursor → designer ∥ qa→dev |
| 2026-08-11 | PM adopted CHL-0001/2/3 (create_refused, epaper platform, floating ToolChip) |
