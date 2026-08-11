---
id: CHL-0008
title: Total architecture rework — stop patching Smart Group sync/selection
status: open
severity: high
raised_by: sm
iter: iter-003
date: 2026-08-11
related: [REQ-03, REQ-04, ADR-0011, ADR-0013, SRS-EP-04, SRS-IN-13, CHL-0004, CHL-0005, CHL-0006, CHL-0007, TRACK-003]
expedite: true
interrupts_track: TRACK-003
---

# CHL-0008 — Architecture rework (human directive)

## Conflict

Human **restored implementation to latest git commit** and directed: **total re-work on the architecture** — not another verify-fix / CHL-0004…0007 patch wave on the current design.

Working tree for `Epaper/` + `infini/` matches `HEAD` (`63c3659` Improve SmartGroup). Uncommitted plan artifacts (CHL-0004…0007, late stories EP-008…011 / IN-023…026, handoffs) remain on disk as history only until PM decides.

## Why SM cannot absorb this

| Layer | Owner |
|---|---|
| Product trade-off (keep pilot UX vs redesign sync/selection model) | **PM** |
| New / superseding SRS + ADR | **Architect** |
| Story re-slice after adopted design | **SM** |
| Code | **Dev** (after stories) |

Continuing EP-006 ∥ IN-019 / CHL-0007 hotfixes would fight the restore and the rework directive.

## Ask PM

1. **Adopt / Defer / Reject** this challenge.
2. If **Adopt**: re-lock `execution:` (scope, stop line) — e.g. pause “verified” on current pilot and open an architecture redesign campaign; name what stays (REQ-03/04 UX?) vs what is in play (device ghost vs mutate, snapshot authority, enclose sync).
3. Route **`/architect`** for a green-field (or surgically superseding) design before SM slices implement stories.
4. Confirm disposition of CHL-0004…0007 (superseded by rework vs keep as constraints).

## Freeze (SM)

TRACK-003 paused — see track freeze note. No new implement stories until PM resolution + architect handoff.

## Resolution

_(PM fills)_
