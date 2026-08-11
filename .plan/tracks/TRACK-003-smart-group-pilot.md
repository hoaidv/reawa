---
id: TRACK-003
slug: smart-group-pilot
kind: planned
status: active
iter: iter-003
cursor: "/sm — slice ink-box waves per architect handoff"
goal: "Ink-box pilot: infini REQ-04 + epaper REQ-03 tool modes"
owner: sm
---

# TRACK-003 — Smart Group pilot

## Goal

Deliver the ink-box pilot across both ends: Infini
[REQ-04](../../.docs/modules/infini/prd.md#smart-group) (tool-armed enclose, explicit selection,
move/resize, `inkScaleMode`) and Epaper
[REQ-03](../../.docs/modules/epaper/prd.md#tool-modes) (3-tool toolbar). Decisions in
[ADR-0011](../../.docs/adr/ADR-0011-smart-group.md) as amended by
[ADR-0013](../../.docs/adr/ADR-0013-ink-box-tool-modes.md).

## Scope

- In: infini/vector-document (SRS-IN-10/11/12/14), infini/tablet-sync (SRS-IN-13),
  epaper/tool-modes (SRS-EP-04/05/06)
- In (prerequisite): tree-backed ink ingestion — nothing else can start without it
- Out: DocChrome; full `doc_op` migration; rotation + connectors on a Smart Group; reawa

## Stories

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-IN-010](../iter-003/stories/STORY-IN-010.md) | implement | **draft** | Predates ADR-0013 — AC mentions a propose/accept step that no longer exists; SM to rewrite or supersede |

To slice: ink ingestion prerequisite, RM2 touch spike, 2 design stories, 5 implement stories —
see the [architect handoff](../iter-003/handoffs/2026-08-11-architect-to-sm-ink-box.md).

## Board

[execution-board](../iter-003/execution-board.md)

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after iter-002 retro-gate; await `/pm` |
| 2026-08-11 | PM adopted tool-armed ink-box; scope expanded to `epaper`, lock WIP → 2 |
| 2026-08-11 | Architect: ADR-0013 + 7 SRS sections; cursor → `/sm` |
