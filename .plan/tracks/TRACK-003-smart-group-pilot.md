---
id: TRACK-003
slug: smart-group-pilot
kind: planned
status: active
iter: iter-003
cursor: "W8 NOW — /designer STORY-EP-012 ∥ /dev STORY-EP-013 (latency gate); CHL-0009 blocks document/sync implement"
goal: "Ink-box rework: epaper REQ-04…REQ-07 (device owns the document) + infini REQ-03 one-way sync"
owner: sm
---

# TRACK-003 — Smart Group pilot → document ownership rework

## Goal

Epaper owns the working document in-session: on-device ink-box create + manipulate, one-way sync.
ADRs: [ADR-0014](../../.docs/adr/ADR-0014-document-ownership-inversion.md),
[ADR-0015](../../.docs/adr/ADR-0015-one-way-sync-contract.md).
Pilot ADRs [ADR-0011](../../.docs/adr/ADR-0011-smart-group.md) (semantics) and
[ADR-0013](../../.docs/adr/ADR-0013-ink-box-tool-modes.md) (tool arming) survive where not reversed.

## Stories (re-slice)

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-EP-012](../iter-003/stories/STORY-EP-012.md) | design | **ready** | W8 — selection chrome; hardware spike |
| [STORY-EP-013](../iter-003/stories/STORY-EP-013.md) | implement | **ready** | W8 — latency gate; blocks all REQ-04 implement |
| [STORY-EP-014](../iter-003/stories/STORY-EP-014.md) | implement | **blocked** | W9 — tree + ingestion; CHL-0009 |
| [STORY-EP-015](../iter-003/stories/STORY-EP-015.md) | implement | **blocked** | W9 — undo ring |
| [STORY-IN-027](../iter-003/stories/STORY-IN-027.md) | implement | **draft** | W9 — desktop applier |
| [STORY-EP-016](../iter-003/stories/STORY-EP-016.md) | implement | **draft** | W10 — enclose |
| [STORY-EP-017](../iter-003/stories/STORY-EP-017.md) | implement | **draft** | W10 — membership |
| [STORY-EP-018](../iter-003/stories/STORY-EP-018.md) | implement | **draft** | W10 — selection-create; depends EP-012 |
| [STORY-EP-019](../iter-003/stories/STORY-EP-019.md) | implement | **draft** | W11 — live manipulation + conformance |
| [STORY-EP-020](../iter-003/stories/STORY-EP-020.md) | implement | **blocked** | W12 — device sync; CHL-0009 |
| [STORY-IN-028](../iter-003/stories/STORY-IN-028.md) | implement | **draft** | W12 — desktop `doc_load` handshake |

Pilot stories (IN-010…019, EP-003…006) remain **done** as history. Residue EP-007…011 / IN-020…026 stays **blocked** — not re-sliced (desktop ink-box authoring is out; device behaviour is the new ids above).

## Board

[execution-board](../iter-003/execution-board.md)

## Freeze note (2026-08-11)

- **Trigger:** Human restored `Epaper/` + `infini/` to latest commit; directed **total architecture rework**.
- **In-flight abandoned:** CHL-0007 hotfixes (not in tree); W7 verify-fix (EP-006 / IN-019) and any CHL-0004…0006 code paths beyond HEAD.
- **Plan residue (uncommitted):** challenges CHL-0004…0007, stories EP-008…011 / IN-023…026, related handoffs — keep as evidence; do not schedule until PM/architect.
- **Risks if resumed blindly:** patch stack on ADR-0013 selection/ghost/snapshot model already failed human verify repeatedly (residue, snap-back, enclose desync, GUI freeze from mid-drag rasterize).
- **Resume checklist:** PM resolves CHL-0008 → Architect redesign/ADR → SM re-slice → QA → Dev. Re-run gates on both modules before calling pilot verified.

**2026-08-13:** Checklist completed through SM re-slice. Track **unpaused**. W7 still void.

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
| 2026-08-13 | PM **adopted** CHL-0008 — Epaper owns the document; one-way sync. Pilot slicing void; await architect |
| 2026-08-13 | Architect delivered ADR-0014 + ADR-0015, the shared domain doc, `SRS-EP-07`…`SRS-EP-14`, device BDD, and both architecture views — [handoff to SM](../iter-003/handoffs/2026-08-13-architect-to-sm-device-document.md) |
| 2026-08-13 | SM re-sliced W8–W12 (EP-012…020, IN-027…028). Opened [CHL-0009](../iter-003/challenges/CHL-0009-missing-device-document-srs-logic.md) for missing `srs-logic.md`. Track **active**. Cursor → designer EP-012 ∥ dev EP-013 |
