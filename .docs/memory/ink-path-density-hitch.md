---
captured: 2026-08-31
related:
  - SRS-EP-01
  - SRS-EP-03
  - SRS-EP-13
  - CHL-0029
  - CHL-0030
---

# Dense-document ink hitch — first millimetre then smooth

Field: after erase phase (EP-062…068), more ink-boxes + free inks make **new** strokes hitch ~100 ms then run smooth. Happens near *and* far from boxes. Recognizers and hand-touch off.

## Cause (confirmed on device)

GUI-thread `rasterizeVectors` FullClear (680–946 ms) between strokes. Next pen-down is queued (`reason=queued behind=rasterizeVectors`). `InkStrokeOperation` and recognizers are not the stall. The 180 ms settle follow-up stole downs after handwriting pauses.

## Fix (2026-08-31)

- Ordinary `RecogOutcome::Ink`: skip Tablet rasterize (live stamps are the settle). [CHL-0029](../../.plan/iter-005/challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)
- No 180 ms follow-up FullClear. Camera still coalesces at 250 ms.
- Structural ops: `InPlaceDirty` of the changed AABB (painter clip + tight `worldClip`); FullClear if AABB missing/huge.
- Recog blink + membership bold: ToolCanvas [`NodeEmphasis`](../../epaper/drawing/tools/ui/node_emphasis.hpp) (Mono, partial AABB). [CHL-0030](../../.plan/iter-005/challenges/CHL-0030-node-emphasis.md)

Guided review: [`.plan/iter-005/handoffs/2026-08-31-dev-guided-review-rasterize-dirty.md`](../../.plan/iter-005/handoffs/2026-08-31-dev-guided-review-rasterize-dirty.md)

## How to attribute

Always-on probe: `/tmp/epaper-ink-path.log` (stderr too). `EPAPER_INK_PATH=0` off.

- `reason=queued i=0 event=down behind=rasterizeVectors` — rasterize stole the first sample (should be gone on ordinary ink after this fix)
- `reason=slow_sample slowest=flushPending|syncPoint|ingestDoc|tabletPaint` — the callback itself
