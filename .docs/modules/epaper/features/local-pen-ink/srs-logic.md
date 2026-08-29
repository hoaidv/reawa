---
feature: local-pen-ink
lifecycle: active
module: epaper
---

# SRS — Local pen-matched ink (logic)

## [SRS-EP-01] Pen event path, coordinate map, and Pen-mode refresh
<!-- lifecycle: active -->

**Implements:** [REQ-01](../../prd.md#local-pen-ink)

1. Run with `QT_QPA_PLATFORM=epaper`, `QT_QUICK_BACKEND=epaper`, and
   `QT_QPA_GENERIC_PLUGINS=evdevtablet`.
2. Deliver tablet (and synthesized mouse) events to a canvas item via an
   application event filter on the GUI thread.
3. Keep the input/ink item **visible** with real geometry.
   `visible: false` collapses width/height and maps all ink to the origin.
   Size the item by binding `width`/`height` to the window. `anchors.fill: parent`
   must not be used: the window's content root is never resized under this QPA, so
   the item stays `0x0`, `updatePaintNode()` returns null, and `paint()` is never
   called — ink and the Pen-mode region both silently vanish.
4. Map digitizer coords into panel scene space for landscape use on a portrait
   panel (`w×h`):

   ```
   renderX = penY * (w / h)
   renderY = h - penX * (h / w)
   ```

5. Render ink with a **single** `QQuickPaintedItem` backed by a persistent
   `QImage`. Damage only the segment bounding box via `update(rect)` — do not
   maintain a large pool of QML `Rectangle` delegates and do not call
   full-window `update()` on every sample.
6. Flush pending ink on a **time budget** (≈ 8 ms), not a distance gate, so
   slow strokes do not accumulate tens of ms before first paint.
7. When `libqsgepaper` exports are available, attach an `EPScreenModeItem` with
   `Mode::Pen` covering the **full ink region**, and keep it resized with the
   canvas. This alone selects the fast waveform; an explicit
   `EPFramebuffer::swapBuffers(rect, Pen, flags)` is not required and stays
   opt-in behind `RM_EP_SWAP`. If symbols are missing, fall back to stock Qt
   epaper refreshes without failing the app.
8. Optional RM→macOS stroke sync (`StrokeSync`) must be inert unless
   `RM_SYNC_HOST` is set, and must not perform socket I/O on the pen hot path.
9. Keep status/debug text off the stroke hot path — refresh it between strokes
   so it never adds a second damage region while drawing.

### Latency quality target

| Metric | Target | Measured 2026-08-09 |
|---|---|---|
| Pen-down → visible ink (subjective) | < 27 ms | **Met** — at parity with xochitl by eye |
| Event arrival → flush | ≪ budget | p50 305 µs, p95 798 µs, p99 1517 µs (n=764) |
| Instrumentation | `RM_INK_TRACE=1` | arrival→flush and flush→swap percentiles on exit |

When ink is invisible, `RM_INK_BEACON=1` stamps a static and a per-flush probe
square into the ink image; together with the `paint=` counter this distinguishes
"Qt never called `paint()`" from "the panel never refreshed".

**Out of scope for this SRS:** macOS stroke ingest UI, viewport sync, pressure polish.

**See also:** [epaper/RENDERING.md](../../../../epaper/RENDERING.md)

---

## [SRS-EP-27] Hardware eraser-nib stroke-erase {#srs-ep-27-eraser-nib}

<!-- lifecycle: retired -->
<!-- superseded-by: [SRS-EP-56] -->
<!-- note: 2026-08-29 CHL-0028 / ADR-0034. Path A sample-delete retired. Brush is SRS-EP-56. -->

**Retired.** Do not implement. Canonical: [prd-erase.md](../../prd-erase.md) · [SRS-EP-56](../erase/srs-logic.md#srs-ep-56-brush).
