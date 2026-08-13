---
title: Execution board — iter-003
iter: iter-003
track: TRACK-003
owner: sm
date: 2026-08-13
lock: vertical · verified · epaper/{device-document, ink-box, tool-modes} + infini/tablet-sync — re-locked 2026-08-13
verdict: "DESIGN COMPLETE — ADR-0014 + ADR-0015 + SRS-EP-07…14 landed; /sm re-slices next"
wave: —
---

# Execution board — iter-003

**Canonical board** for [TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) — **paused for
re-slice**.

## Summary (as of 2026-08-13)

| Band | Count | Meaning |
|---|---|---|
| W0–W6 | **done** | Pilot slice at git HEAD — kept as history; its ownership model is withdrawn |
| W7 verify-fix | **void** | Superseded by CHL-0008 (code restored; no patch wave) |
| Patch-wave residue | **blocked** | EP-007…011, IN-020…026 — assumed desktop authority |
| Architecture rework | **done** | ADR-0014, ADR-0015, domain doc, SRS-EP-07…14, BDD, architecture views |
| Re-slice | **next** | `/sm` — [architect handoff](./handoffs/2026-08-13-architect-to-sm-device-document.md) |

## Lock (re-locked 2026-08-13, CHL-0008 adopted)

```
direction: vertical
stop_line: verified
autonomy: bounded
out_of_scope: backlog
wip: 2
modules: epaper, infini
features: epaper/device-document; epaper/ink-box; epaper/tool-modes; infini/tablet-sync
personas: /sm NOW (re-slice) → /designer (SRS-EP-12) → /qa → /dev
forbidden: /dev on any story assuming desktop tree authority; any design reintroducing a
           peer round trip inside an editing gesture
NOW: /sm — see 2026-08-13-architect-to-sm-device-document.md
cursor: re-slice on the device SRS
```

## Waves

| Wave | Status | What |
|---|---|---|
| W0–W6 | **done** | Shipped at HEAD (pilot model, now withdrawn) |
| W7 | **void** | EP-006 ∥ IN-019 verify-fix — do not pull |
| Rework design | **done** | ADR-0014 + ADR-0015 + domain doc + SRS-EP-07…14 + BDD |
| Rework build | **not sliced** | SM re-slices now, per the architect handoff |

## Full task table

| Id | Status | Next owner | Note |
|---|---|---|---|
| CHL-0008 | **resolved / adopted** | — | [Resolution](./challenges/CHL-0008-architecture-rework.md) 2026-08-13 |
| PM PRD rework | **done** | — | epaper v0.5.0 · infini v0.4.0 · [handoff](./handoffs/2026-08-13-pm-to-architect-device-document.md) |
| Architect rework | **done** | — | ADR-0014, ADR-0015, [domain doc](../../.docs/domain/vector-document.md), SRS-EP-07…14, epaper architecture · [handoff](./handoffs/2026-08-13-architect-to-sm-device-document.md) |
| Re-slice TRACK-003 | **next** | `/sm` | Design story for [SRS-EP-12] first; ink-latency measurement before any REQ-04 implement story |
| STORY-EP-006 / IN-019 | done (parked) | — | Do not re-verify against the old model |
| EP-007…011, IN-020…026 | **blocked** | — | Superseded by the rework; re-specified from scratch |

## Verdict

Design is complete and the run is unblocked for slicing. No `/dev` until SM re-slices against
`SRS-EP-07…14`. The design-coverage gate FAIL for epaper `[REQ-05]` / `[REQ-06]` / `[REQ-08]` and
the missing-story FAIL for `SRS-EP-09…14` are **expected** and close with the re-slice — they are
listed in the architect handoff so they are not mistaken for regressions.
