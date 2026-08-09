---
id: CHG-0003
title: Pen-mode partial updates and single-node ink path
date: 2026-08-09
iter: iter-001
source: EXP-0001
---

# CHG-0003 — Epaper latency path (Pen mode)

## Decision

Bind `libqsgepaper` at runtime (`dlsym`) for `EPScreenModeItem(Mode::Pen)` and
`EPFramebuffer::swapBuffers`. Replace the 2000-item QML rectangle pool with a
single `QQuickPaintedItem` + time-based flush. Keep stroke TCP off the hot path
unless `RM_SYNC_HOST` is set.

## Outcome

Met the goal: pen-to-ink is at parity with xochitl by eye (< 27 ms).
`RM_INK_TRACE` measured arrival→flush at p50 305 µs / p95 798 µs (n=764), so the
ink layer is no longer a meaningful contributor.

The blocking defect was **not** in the epaper backend. `anchors.fill: parent`
left the canvas `0x0` because the `QQuickRootItem` of our `QQuickWindow`
subclass is never resized under this QPA, so `updatePaintNode()` returned null
and `paint()` never ran — and the Pen-mode region was tagged over an empty rect.
Binding `width`/`height` to the window fixed both the invisible ink and the
waveform. Explicit `swapBuffers` turned out to be unnecessary and is now opt-in
behind `RM_EP_SWAP`.

Also fixed: a double free of the placement-constructed `EPScreenModeItem`
storage (Qt already destroys it via parent ownership), and `SIGTERM` is now
routed through the event loop so trace stats survive `killall`.

## Product docs

- [SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) extended with Pen-mode + latency target
- Explainer: [epaper/RENDERING.md](../../../epaper/RENDERING.md)

## Code

- `epaper/epaperbridge.{h,cpp}`
- `epaper/tabletcanvasitem.{h,cpp}`, `Main.qml`, `strokesync.*`, `tabletwindow.*`
