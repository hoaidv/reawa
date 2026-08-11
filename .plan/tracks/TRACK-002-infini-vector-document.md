---
id: TRACK-002
slug: infini-vector-document
kind: planned
status: active
iter: iter-002
cursor: "/qa BDD for W4 Must → /dev STORY-IN-007 (tree/ops)"
goal: "Document model + sync implement (vector-doc with tablet/region sync); DocChrome design cancelled"
owner: sm
---

# TRACK-002 — Infini vector document (+ sync wave)

## Goal

Deliver [REQ-02](../../.docs/modules/infini/prd.md#vector-document) / [REQ-03](../../.docs/modules/infini/prd.md#tablet-sync)
and Epaper [REQ-02](../../.docs/modules/epaper/prd.md#region-sync) implement together (W4).
[REQ-04](../../.docs/modules/infini/prd.md#smart-group) Could rides after Must.
Open/save chrome **design cancelled** (STORY-IN-006).

## Scope

- In: Implement stories IN-007…010 + EP-001; QA BDD then Dev in depends_on order
- Out: `/designer` STORY-IN-006; on-device Smart Group on Epaper v0

## Stories

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-IN-006](../iter-002/stories/STORY-IN-006.md) | design | **blocked** (cancelled) | No design per human |
| [STORY-IN-007](../iter-002/stories/STORY-IN-007.md) | implement | **ready** | SRS-IN-04 tree/ops — **NOW** |
| [STORY-IN-008](../iter-002/stories/STORY-IN-008.md) | implement | **ready** | SRS-IN-09 SVG + fixtures |
| [STORY-IN-009](../iter-002/stories/STORY-IN-009.md) | implement | **ready** | SRS-IN-07 Infini session |
| [STORY-EP-001](../iter-002/stories/STORY-EP-001.md) | implement | **ready** | SRS-EP-02 Epaper region sync |
| [STORY-IN-010](../iter-002/stories/STORY-IN-010.md) | implement | **draft** | SRS-IN-10 Smart Group Could |

## Board

[execution-board](../iter-002/execution-board.md)

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after F1 gate; W3 was STORY-IN-006 /designer |
| 2026-08-11 | Human: no design; cursor → `/architect`; no `/dev` until sync |
| 2026-08-11 | Architect READY-WITH-CONCERNS; W3-arch closed; W4 slices opened |
