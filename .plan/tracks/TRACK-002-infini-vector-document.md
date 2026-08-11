---
id: TRACK-002
slug: infini-vector-document
kind: planned
status: active
iter: iter-002
cursor: "vector-document SRS /architect (sync readiness) — no designer; no dev yet"
goal: "Document model + ADRs ready for epaper↔Infini sync; implement deferred to sync wave"
owner: sm
---

# TRACK-002 — Infini vector document

## Goal

Hold [REQ-02](../../.docs/modules/infini/prd.md#vector-document) / [REQ-04](../../.docs/modules/infini/prd.md#smart-group)
architecture (tree-of-vectors, Smart Group) ready for the **sync** wave. Open/save chrome
**design cancelled**; implement stories **not** opened until epaper ↔ desktop sync build.

## Scope

- In: Architect review / bind document ops to session channels; keep SRS/ADRs coherent
- Out (this wave): `/designer` STORY-IN-006; `/dev` implement of tree/SVG/chrome/Smart Group
- Later: implement with tablet-sync / region-sync

## Stories

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-IN-006](../iter-002/stories/STORY-IN-006.md) | design | **blocked** (cancelled) | No design per human |

Implement slices: **not created yet** (defer to sync).

## Board

[execution-board](../iter-002/execution-board.md)

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after F1 gate; W3 was STORY-IN-006 /designer |
| 2026-08-11 | Human: no design; cursor → `/architect`; no `/dev` until sync |
