---
from: architect
to: sm
date: 2026-08-13
iter: iter-003
source: STORY-EP-012
cc: [pm, designer, qa, dev]
verdict: READY-WITH-CONCERNS
---

# Hand-off: Architect → SM — EP-012 device units accepted

## Verdict

**READY-WITH-CONCERNS.** The EP-012 Spec proposals are **accepted** as implement locks for
[STORY-EP-019](../stories/STORY-EP-019.md). Say so explicitly: EP-019 **may lock** these numbers.
Do **not** fall back to 8 CSS px or `TILE_LOD_SCALE = 0.35`.

| Constant | Value | Coordinate system |
|---|---|---|
| Handle visual | **28 du** (≈3.1 mm @ 226 dpi) | 1 du = 1 panel pixel |
| Handle hit | **56 du** (≈6.3 mm; 14 du pad beyond visual) | same |
| LOD cutoff | min on-panel axis **< 96 du** (≈10.8 mm) | on-panel axis, **not** viewport scale |

## Why accept (not hardware-block)

The original Open rows existed to stop **desktop constants** (mouse 8 CSS px; tile LOD 0.35) from
landing on a fixed 1-bit panel. The Spec spike re-derived in **device units** against known RM2
geometry (1404×1872 @ 226 dpi):

- Pen handles are not finger chip tiles (64×64) and not mouse 8 px. 56 du hit is ~6.3 mm — EMR
  tracking is sub-mm; visual 28 du reads as filled chrome vs 1-bit ink strokes.
- LOD is **min on-panel axis**, because the device has no tiles and no on-device pan. Below 96 du,
  opposite 28 du handles collide. Press below cutoff falls through to **nothing** (not pan).

That is enough to lock implement. A miss-rate measurement on hardware is **QA**, not a reason to
leave the rows open or to invent 8 px / 0.35.

## Concern (accepted)

Values are DPI + collision math, not a measured pen miss-rate on RM2. If EP-019 device QA shows
systematic misses, file a `CHL-*` with a **new du number**. Never reopen 8 CSS px or 0.35.

## Docs closed

- [SRS-EP-12 Open](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#open-needs-design) —
  handle + LOD rows closed.
- [SRS-EP-11 hit-testing](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) — 28/56/96
  bound; “Open — device constants” closed.
- [REQ-04 Open Questions](../../../.docs/modules/epaper/prd.md#open-questions) — LOD bullet closed.
- [epaper architecture risk](../../../.docs/modules/epaper/architecture.md) — desktop-constant risk
  marked closed.
- Spec spike table — one-line lock note
  ([ui-spec.md](../design/device-selection-chrome/ui-spec.md#spike)). HTML scenes unchanged.

No new SRS ids. ADR-0014 / ADR-0015 not rewritten. No `epaper/` / `Epaper/` code.

## Asks

1. EP-019 may treat 28/56/96 as locked. Story AC should cite **min on-panel axis ≥ 96 du**, not
   “viewport scale” / `TILE_LOD_SCALE`. Architect does not edit stories.
2. Do **not** flip EP-018 / EP-019 to `ready` until the wave allows (EP-013 device p95 first;
   lock: `/dev` on REQ-04 implement only after EP-013 passes). This confirm does **not** ungate W9.
3. CHL-0010 is **PM deferred** (separate handoff) — not this confirm. EP-018 stays frozen `draft`.
4. `/qa` still owns EP-013 — `NOW: /qa STORY-EP-013`.

## Review (review-design)

| Class | Finding |
|---|---|
| Strength | Correct coordinate system (panel du); LOD is on-panel axis, not tile scale |
| Concern | Unmeasured miss-rate — accepted; tune via CHL in du |
| Risk | None new. Desktop-constant inheritance is closed |

## Out of scope

- CHL-0010 triage (PM, already deferred).
- Design package HTML.
- W9 ungate. EP-018 / EP-019 `ready`.
- New SRS ids, ADR rewrites, `epaper/` code.
