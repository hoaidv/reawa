---
id: TRACK-003
slug: smart-group-pilot
kind: planned
status: active
iter: iter-003
cursor: "/qa→/dev W4 (IN-015) · EP-005 waits on IN-018"
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
| [STORY-IN-012](../iter-003/stories/STORY-IN-012.md) | implement | **ready** | ink ingestion prerequisite |
| [STORY-EP-004](../iter-003/stories/STORY-EP-004.md) | implement | **ready** | RM2 touch spike |
| [STORY-IN-013](../iter-003/stories/STORY-IN-013.md) | design | **ready** | ink-box-ui |
| [STORY-IN-014](../iter-003/stories/STORY-IN-014.md) | implement | ready | undo |
| [STORY-IN-015](../iter-003/stories/STORY-IN-015.md) | implement | draft | selection + UV |
| [STORY-IN-010](../iter-003/stories/STORY-IN-010.md) | implement | draft | enclose (rewritten) |
| [STORY-IN-016](../iter-003/stories/STORY-IN-016.md) | implement | draft | membership |
| [STORY-IN-017](../iter-003/stories/STORY-IN-017.md) | implement | draft | surround create |
| [STORY-EP-003](../iter-003/stories/STORY-EP-003.md) | design | draft | epaper strip (spike-gated) |
| [STORY-IN-018](../iter-003/stories/STORY-IN-018.md) | implement | draft | transport |
| [STORY-EP-005](../iter-003/stories/STORY-EP-005.md) | implement | draft | device tools |

## Board

[execution-board](../iter-003/execution-board.md)

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after iter-002 retro-gate |
| 2026-08-11 | PM ink-box UX; scope + epaper; WIP 2 |
| 2026-08-11 | Architect ADR-0013 + SRS; then confirm IN-15/16 + UV |
| 2026-08-11 | SM sliced 11 stories; cursor → designer ∥ qa→dev |
