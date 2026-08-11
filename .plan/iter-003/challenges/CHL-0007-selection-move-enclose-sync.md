---
id: CHL-0007
title: Selection residue, move snap-back, consecutive enclose desync
status: resolved
resolution: adopted
severity: normal
raised_by: architect
resolved_by: architect
iter: iter-003
date: 2026-08-11
related: [SRS-EP-04, SRS-EP-01, SRS-IN-13, REQ-03, REQ-04, CHL-0006]
---

# CHL-0007 — Selection / move / enclose sync (human verify bugs)

## Conflict

Human verify after TRACK-003 Smart Group work:

1. Selection outlines **accumulate** on e-ink.
2. Move: ghost follows, pen-up **snap-back flash**, then ink jumps.
3. Consecutive enclose often creates only the first ink-box; then pan/zoom/move desync (selection rect correct, ink wrong).

## Prior design gaps

| Item | Gap |
|---|---|
| Selection chrome | Dirty region was new rect only → old chrome ghosted on e-ink |
| Move commit | Cleared ghost before Infini snapshot; local pickable still at origin → flash |
| Failed enclose | Infini skipped `doc_snapshot` on `ordinary_ink` (race avoidance) → tablet kept local stroke while authority diverged |
| Mid-input snapshot | Drop/ignore mid-gesture without reliable flush; stuck `m_strokeActive` when release gated on tool mode deferred **all** rasterize (pan/zoom look dead; pickables still update) |

## Resolution

**Adopted** 2026-08-11 (implementation + SRS amend same day).

1. Damage selection chrome as **old∪new**.
2. Move pen-up: optimistic local path/bounds translate + rasterize, then `tool_intent`.
3. Queue (never drop) `doc_snapshot` during stroke **or** selection gesture; apply on pen-up.
4. Pen-up always ends active stroke; tool-arm aborts in-flight stroke/gesture.
5. Infini pushes `doc_snapshot` for enclose `created` **and** `ordinary_ink`.
6. `stroke_point` may carry `intent` for enclose recovery.

Stories: fold into EP-006 / IN-019 verify fix wave (or SM splits EP-012 / IN-027 if preferred).
