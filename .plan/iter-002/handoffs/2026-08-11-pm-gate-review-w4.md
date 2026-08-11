---
from: pm
to: sm
iter: iter-002
date: 2026-08-11
subject: w4-must-gate
verdict: READY-WITH-CONCERNS
cc: [qa, architect, dev]
---

# Gate review — W4 Must (vector-doc + tablet/region sync)

## Verdict: **READY-WITH-CONCERNS**

W4 **Must** stories are `done`. Human confirmed RM2→Infini draw on hardware (2026-08-11).
Campaign exit criteria for Must sync path are met with owned gaps below — **not** a clean
`adlc gate --check` green (repo-wide / out-of-scope noise).

## Scope checked (lock)

```
direction: vertical · modules: infini, epaper · stop: verified · wip: 1
features: infinity-canvas; vector-document; tablet-sync; region-sync
```

| REQ | Feature | Evidence | Status |
|---|---|---|---|
| Infini REQ-01 | infinity-canvas | STORY-IN-001…005 done (prior wave) | **met** |
| Infini REQ-02 | vector-document | IN-007/008 done; DocChrome design cancelled | **met (chrome deferred)** |
| Infini REQ-03 | tablet-sync | IN-009 done + live StrokeSync ingest | **met (ADR-0009 wire partial)** |
| Epaper REQ-02 | region-sync | EP-001 done + human draw confirmed | **met (Qt RegionSession wire follow-up)** |
| Infini REQ-04 | Smart Group Could | STORY-IN-010 draft | **out of Must** |

`validated_by: human / 2026-08-11` (draw sync observed).

## Story rollup (W4 Must)

| Story | Status |
|---|---|
| STORY-IN-007 | done |
| STORY-IN-008 | done |
| STORY-IN-009 | done |
| STORY-EP-001 | done |
| STORY-IN-006 | blocked (cancelled — do not revive) |
| STORY-IN-010 | draft Could — not gated |

## Engine (`adlc gate`) — paste summary

**Overall:** FAIL (21 checks; ~15 pass). Failures relevant to this campaign vs noise:

| FAIL | Disposition |
|---|---|
| Orphan SRS-IN-05 (DocChrome) | **Owned** — design cancelled; no implement chrome this campaign |
| Orphan SRS-IN-10 / Needs-design REQ-04 | **Owned** — Could pilot; IN-010 not opened |
| SRS-EP-01 / quality SRS without dedicated story | **Owned** — EP-01 prior; EP-03/IN-08 tagged on sibling BDD |
| `reawa/*` design/BDD/story gaps | **Out of lock** — backlog; do not block Infini↔Epaper gate |
| tablet-sync srs-ui without design story | **Owned** — REQ-03 Needs design: **no** |

`prd-check` infini + epaper: **ok**. Lock checks: **PASS**.

## Concerns (do not block Must gate)

1. **Reconnect `snapshot`/`hello`** still TBD (architect READY-WITH-CONCERNS).
2. **Live path** used EXP `StrokeSync`→Infini `:9877`, not full ADR-0009 Qt `RegionSession` in `TabletCanvasItem`.
3. **DocChrome / SRS-IN-05** remains cancelled.
4. **Smart Group (REQ-04 / IN-010)** still Could — Needs design: yes unpaid until pilot opens.

## Ask of SM

1. Advance [execution-board](../execution-board.md): W4 Must → **done**; F2/F3/F4 verified for Must.
2. Update TRACK-002 cursor (gate closed / IN-010 decision).
3. Ask human: open **STORY-IN-010** next, or park Could and prepare iter close / next campaign slice.

## Next

**`/sm`** — board + track update. Optional **`/dev`** only if human opens IN-010 (after `/qa` BDD).
