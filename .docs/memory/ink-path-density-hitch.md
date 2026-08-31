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

## Follow-up (2026-08-31, same session)

Device log after the FullClear skip, recognizers off:

- Downs at the **end** of a dense page: `behind=toolPaint` 77–357 ms, not `rasterizeVectors`.
- Long stroke (~every 100 samples, `gap_ms≈100`): another `toolPaint` ~90–105 ms. Ink goes solid → dash-dash-dash → solid.
- Recog lines still `fail=recog_off`, but **draw-into membership still runs** and `NodeEmphasis` Bold keeps ToolCanvas visible + Mono. `paint()` HierarchyCull’d the group AABB and re-stroked every overlapping ink in Mono.

Fix: hide ToolCanvas while an ink stroke is active; `includeIds` so emphasis paints only blink/stamp ids; `clearStrokeStamp` syncs overlay off.

## Follow-up (2026-08-31) — camera settle on pen-up

Latest log after overlay hide: many downs are `slow_sample rasterize.render` 24–120 ms **inside** the pointer callback (`inplace true`). `endStroke` 438–624 ms when camera-pending rasterize ran synchronously on pen-up. Ordinary ink now **queues** that camera pass on the 250 ms timer instead of running it on up.

Move/resize downs still rasterize (origin punch). That punch must include bound connector spines or the origin connector remains on TabletCanvas.

## How to attribute

Always-on probe: `/tmp/epaper-ink-path.log` (stderr too). `EPAPER_INK_PATH=0` off.

- `reason=queued i=0 event=down behind=rasterizeVectors` — rasterize stole the first sample
- `reason=queued behind=toolPaint` — ToolCanvas Mono overlay stole the sample (membership/blink)
- `reason=slow_sample slowest=flushPending|syncPoint|ingestDoc|tabletPaint` — the callback itself
