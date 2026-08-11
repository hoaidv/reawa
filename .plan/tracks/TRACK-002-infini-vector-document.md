---
id: TRACK-002
slug: infini-vector-document
kind: planned
status: active
iter: iter-002
cursor: "/pm W5 gate — IN-011+EP-002 done (PASS-WITH-CONCERNS; human hardware confirm)"
goal: "W5 live viewport path done (vector settle); W4 Must done; IN-010 Could parked"
owner: sm
---

# TRACK-002 — Infini vector document (+ sync wave)

## Goal

Deliver [REQ-02](../../.docs/modules/infini/prd.md#vector-document) / [REQ-03](../../.docs/modules/infini/prd.md#tablet-sync)
and Epaper [REQ-02](../../.docs/modules/epaper/prd.md#region-sync) implement together (W4).
[REQ-04](../../.docs/modules/infini/prd.md#smart-group) Could rides after Must.
Open/save chrome **design cancelled** (STORY-IN-006).

## Scope

- In: W5 IN-011 + EP-002 (live viewport); prior W4 Must done
- Out: `/designer` STORY-IN-006; on-device Smart Group; IN-010 until unparked

## Stories

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-IN-006](../iter-002/stories/STORY-IN-006.md) | design | **blocked** (cancelled) | No design per human |
| [STORY-IN-007](../iter-002/stories/STORY-IN-007.md) | implement | **done** | W4 tree/ops |
| [STORY-IN-008](../iter-002/stories/STORY-IN-008.md) | implement | **done** | W4 SVG + fixtures |
| [STORY-IN-009](../iter-002/stories/STORY-IN-009.md) | implement | **done** | W4 Infini session |
| [STORY-EP-001](../iter-002/stories/STORY-EP-001.md) | implement | **done** | W4 Epaper region headers + live draw |
| [STORY-IN-011](../iter-002/stories/STORY-IN-011.md) | implement | **draft** | W5 marker + viewport publish — **NOW after arch** |
| [STORY-EP-002](../iter-002/stories/STORY-EP-002.md) | implement | **draft** | W5 map + e-ink coalesce refresh |
| [STORY-IN-010](../iter-002/stories/STORY-IN-010.md) | implement | **draft** | SRS-IN-10 Smart Group Could — parked |

## Board

[execution-board](../iter-002/execution-board.md)

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after F1 gate; W3 was STORY-IN-006 /designer |
| 2026-08-11 | Human: no design; cursor → `/architect`; no `/dev` until sync |
| 2026-08-11 | Architect READY-WITH-CONCERNS; W3-arch closed; W4 slices opened |
| 2026-08-11 | W4 Must gated (human draw); W5 opened IN-011 + EP-002; cursor `/architect` |
