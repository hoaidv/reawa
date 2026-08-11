---
id: TRACK-003
slug: smart-group-pilot
kind: planned
status: paused
paused_reason: "CHL-0008 — human restored code to HEAD; total architecture rework (not more patch waves)"
iter: iter-003
cursor: "PAUSED — CHL-0008 architecture rework; await PM adopt + /architect"
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
| [STORY-EP-006](../iter-003/stories/STORY-EP-006.md) | implement | **ready** | verify fix — ToolChip touch |
| [STORY-IN-019](../iter-003/stories/STORY-IN-019.md) | implement | **ready** | verify fix — connection eager sync |

## Board

[execution-board](../iter-003/execution-board.md)

## Freeze note (2026-08-11)

- **Trigger:** Human restored `Epaper/` + `infini/` to latest commit; directed **total architecture rework**.
- **In-flight abandoned:** CHL-0007 hotfixes (not in tree); W7 verify-fix (EP-006 / IN-019) and any CHL-0004…0006 code paths beyond HEAD.
- **Plan residue (uncommitted):** challenges CHL-0004…0007, stories EP-008…011 / IN-023…026, related handoffs — keep as evidence; do not schedule until PM/architect.
- **Risks if resumed blindly:** patch stack on ADR-0013 selection/ghost/snapshot model already failed human verify repeatedly (residue, snap-back, enclose desync, GUI freeze from mid-drag rasterize).
- **Resume checklist:** PM resolves CHL-0008 → Architect redesign/ADR → SM re-slice → QA → Dev. Re-run gates on both modules before calling pilot verified.

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after iter-002 retro-gate |
| 2026-08-11 | PM ink-box UX; scope + epaper; WIP 2 |
| 2026-08-11 | Architect ADR-0013 + SRS; then confirm IN-15/16 + UV |
| 2026-08-11 | SM sliced 11 stories; cursor → designer ∥ qa→dev |
| 2026-08-11 | PM adopted CHL-0001/2/3 (create_refused, epaper platform, floating ToolChip) |
| 2026-08-11 | Human verify FAILED — toolbar touch + late connection; sliced EP-006 + IN-019 |
| 2026-08-11 | **Paused** — CHL-0008 architecture rework; code at HEAD |
