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

<!-- lifecycle: active -->

**Parent:** [REQ-11](../../prd.md#erase) Path A. **Decision:** [ADR-0025](../../../../adr/ADR-0025-barrel-vs-eraser-nib.md). **Barrel `temp_erase`** uses this **mutation** but **not** this HID path ([SRS-EP-41](../tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch)). Path B: [SRS-EP-28](../device-document/srs-logic.md#srs-ep-28-selection-erase).

| Rule | Value |
|---|---|
| Trigger | Digitizer reports a distinct **eraser tool** / inverted nib (not barrel button) |
| During rub | Remove **ink samples** whose world position intersects the nib footprint (radius start: **8 u** or reported width). Partial-refresh damaged AABB only |
| After gesture | p95 ≤50 ms until intersecting samples are gone ([SRS-EP-30](./srs-quality.md#srs-ep-30-erase-quality)) |
| New Ink | **0** Ink nodes created |
| Empty node | If an Ink (or other sample-holding) node has **no remaining samples**, `remove_node` that node |
| Undo | One undo restores the pre-erase document via inverse of `set_ink_samples` and, if the forward also `remove_node`’d an emptied ink, restore of that body. Exactness when `lastOpId` matches ([SRS-EP-07](../device-document/srs-logic.md#srs-ep-07-device-document)); geometry ±1 px @ 100% zoom vs stored samples |
| No nib | Path A **does not fire** (0 accidental erases). Pen tip never erases |
| No session | Same local result; publish `set_ink_samples` and/or `remove_node` (`compound` if both) when linked ([REQ-07](../../prd.md#one-way-sync)). Never `restore_snapshot` |
| Ink latency | Must not put I/O on the pen-tip hot path ([SRS-EP-01](#srs-ep-01)); eraser nib is a **different** tool report |

### UI-driving fields

| Field | Drives |
|---|---|
| `stylus.tool` | `pen` \| `eraser_nib` |
| `erase.in_progress` | [SRS-EP-29](../tool-modes/srs-ui.md#srs-ep-29-erase-ui) nib-in-progress state |
