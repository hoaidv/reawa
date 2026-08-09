---
feature: local-pen-ink
lifecycle: active
module: epaper
---

# SRS — Local pen-matched ink (logic)

## [SRS-EP-01] Pen event path and coordinate map
<!-- lifecycle: active -->

**Implements:** [REQ-01](../../prd.md#local-pen-ink)

1. Run with `QT_QPA_PLATFORM=epaper`, `QT_QUICK_BACKEND=epaper`, and
   `QT_QPA_GENERIC_PLUGINS=evdevtablet`.
2. Deliver tablet (and synthesized mouse) events to a canvas item via
   `QQuickWindow::tabletEvent` and/or an application event filter.
3. Keep the input item **visible** with real geometry (transparent fill is fine).
   `visible: false` collapses width/height and maps all ink to the origin.
4. Map digitizer coords into panel scene space for landscape use on a portrait
   panel (`w×h`):

   ```
   renderX = penY * (w / h)
   renderY = h - penX * (h / w)
   ```

5. Render ink by **moving** pre-created QML `Rectangle` nodes (pool), not by
   inserting new delegates per point and not by calling full-window
   `update()` on every sample.
6. Throttle segment emission to a minimum pen travel (≈ 3 px) so the e-ink
   refresh queue is not starved.

**Out of scope for this SRS:** macOS stroke ingest, viewport sync, pressure polish.
