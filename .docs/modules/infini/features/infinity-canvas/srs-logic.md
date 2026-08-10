---
feature: infinity-canvas
parent_req: [REQ-01]
version: 0.1.0
lifecycle: active
---

# SRS — Infinity canvas (Logic)

## [SRS-IN-01] Canvas transform model

### Other logic

- World space is unbounded 2D; the window shows an axis-aligned view.
- Transform: `screen = (world + translate) * scale` with `scale > 0` uniform (no non-uniform scale, no rotate in v0).
- Inverse: `world = screen / scale - translate` used for hit-testing and for computing the drawing-region AABB sent to Epaper (when tablet-sync is active).
- Gesture inputs (trackpad pan, mouse drag pan, wheel pan, modifier+wheel pan/zoom, trackpad pinch zoom) update `translate` / `scale` only; they do not mutate document geometry.
- Primitive figures (line, rect, ellipse/circle, path) are stored in world space; the renderer applies the canvas transform at draw time.

### Invariants

- Circle world geometry remains circular on screen under any allowed pan/zoom.
- Zoom focuses on gesture focal point when the platform provides it; otherwise document window center (document in architecture spike).
