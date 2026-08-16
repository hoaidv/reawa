---
from: sm
to: dev
date: 2026-08-16
iter: iter-004
cc: [qa]
---

# Handoff — SM → Dev — EP-035 is NOW

## Context

PM gated USB + Infini work **done** (EP-034, EP-036, IN-030, IN-032). TRACK-004 cursor is
[STORY-EP-035](../stories/STORY-EP-035.md) — `ready` · P1 · implement · owner `dev`.

Human 2026-08-16: handwriting `"his"` became an ink-box. **Do not** change containment / PIP.
Measure **area/length** of the stroke as a closed ring vs fat empty boundary (O, D, fat-W).

Capture table: [ep-035-area-length.md](../notes/ep-035-area-length.md).

## Asks

1. Log `L`, `A` (shoelace of last→first ring), `A/L`, `A/L²` on `[recog]` / `encloseWhy`.
2. Host print for his-like vs O vs fat-W-like — **no threshold assert**.
3. Do **not** change enclose verdict, closed-ish, size bars, or content AABB.
4. `@implements [SRS-EP-10] enclose area/length probe`.

Then **`/qa`** for the log/host analog; human fills the notes table on device.

## Constraints

No PIP / hull / self-intersect guard. No SRS retune. Reuse `enclose_shape.hpp` shoelace.
Do not UDC-unplug USB (EP-034/036 already shipped; leave that path alone).

## Out of scope

Shipping an A/L bar. EP-032 chrome ADR. iter-005. Connector `hullAreaOverLen2`.
