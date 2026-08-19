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
- Zoom focuses on gesture focal point when the platform provides it; otherwise **window center**.
- On window resize, the **world point under the window center** stays fixed (SRS-UI locked decision).

### UI-driving fields

| Field | Required | Drives SRS-UI |
|---|---|---|
| `document.vectors.length` | yes ≥0 | `canvas.empty` when 0; `canvas.populated` when ≥1 |
| `viewport.scale` | yes >0 | StatusZoom `{round(scale*100)}%` |
| `viewport.translate` | yes | WorldLayer transform |
| `ui.gesturing` | yes bool | `canvas.gesturing` while true |
| `viewportOwner` | yes `infini` \| `epaper` | Follow vs navigate ([SRS-IN-20](#srs-in-20-follow-viewport)) |

---

## [SRS-IN-20] Follow tablet-published viewport {#srs-in-20-follow-viewport}

<!-- lifecycle: active -->

**Parent:** Epaper [REQ-10](../../../epaper/prd.md#hand-touch) (Infini follow). **Decision:** [ADR-0023](../../../../adr/ADR-0023-viewport-last-writer.md). **Session:** [SRS-IN-21](../tablet-sync/srs-logic.md#srs-in-21-viewport-token). **Do not parent this on [SRS-IN-01](#srs-in-01)** — that section remains Infini-driven gestures.

| Rule | Value |
|---|---|
| When `viewportOwner = epaper` | Apply inbound tablet `viewport` to `translate` / `scale` / drawingRegion; **do not** start a competing Infini pan/pinch publish |
| After settle | Infini view matches tablet region (0 divergent viewports) |
| Infini user starts pan/pinch | Steal token → `infini`; existing SRS-IN-01 gestures apply |
| Idle | Infini may navigate as today |
