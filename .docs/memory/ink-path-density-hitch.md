---
captured: 2026-08-31
related:
  - SRS-EP-01
  - SRS-EP-13
---

# Dense-document ink hitch — first millimetre then smooth

Field: after erase phase (EP-062…068), more ink-boxes + free inks make **new** strokes hitch ~100 ms then run smooth. Happens near *and* far from boxes. Recognizers and hand-touch off.

## Why it is not InkStrokeOperation itself

`InkStrokeOperation` only forwards to `TabletCanvasItem::ingestPen`. Density cannot live in `match()`. The hitch that matches “random, location-independent, then smooth” is GUI-thread work **between** samples, especially a FullClear document rasterize queued from the previous stroke (`kSettleFollowUpMs` 180 / `kRefreshMinIntervalMs` 250). `rasterizeVectors` walks every visible ink (AABB from samples, then draw) and `refreshAllConnectorWarps` walks the tree. Pen-down is queued behind that; remaining samples flush quickly.

Existing `UiStallSection` default bar is **250 ms**, so a 100 ms rasterize is invisible. `RM_INK_TRACE` only measures arrival→flush, not *what* stole the thread.

## How to attribute

Always-on probe: `/tmp/epaper-ink-path.log` (stderr too). `EPAPER_INK_PATH=0` off.

- `reason=queued i=0 event=down behind=rasterizeVectors` — rasterize stole the first sample
- `reason=slow_sample slowest=flushPending|syncPoint|ingestDoc|tabletPaint` — the callback itself
