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
- Inverse: `world = screen / scale - translate` used for hit-testing and for computing the drawing-region AABB sent to Epaper **only while Epaper follow is on**.
- Gesture inputs (trackpad pan, mouse drag pan, wheel pan, modifier+wheel pan/zoom, trackpad pinch zoom) update `translate` / `scale` only; they do not mutate document geometry. If Infini follow is on at gesture start, set `direction = none` **then** drive the local camera ([SRS-IN-26](../tablet-sync/srs-logic.md#srs-in-26-viewport-follow)).
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
| `follow.direction` | yes `none` \| `infini_to_epaper` \| `epaper_to_infini` | Apply vs navigate ([SRS-IN-20](#srs-in-20-follow-viewport)); toggle is [SRS-IN-26](../tablet-sync/srs-logic.md#srs-in-26-viewport-follow) |

---

## [SRS-IN-20] Apply tablet viewport while Infini following {#srs-in-20-follow-viewport}

<!-- lifecycle: active -->
<!-- revised: 2026-08-20 — ADR-0029. Apply inbound tablet viewport only while Infini follow is on.
     Same id; follow toggle is [SRS-IN-26], not a parent of this section. Last-writer withdrawn. -->

**Parent:** [REQ-01](../../prd.md#infinity-canvas) (Infini camera apply). **Gate (not parent):** [REQ-06](../../prd.md#viewport-follow) / [SRS-IN-26](../tablet-sync/srs-logic.md#srs-in-26-viewport-follow). **Decision:** [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md). **Session emit/apply:** [SRS-IN-21](../tablet-sync/srs-logic.md#srs-in-21-viewport-token). **Do not parent this on [SRS-IN-01](#srs-in-01)** — that section remains Infini-driven local gestures. **Do not parent Infini REQ-06 here.**

Local Infini pan/pinch is **Must** and always drives **this** canvas. Last-writer token is withdrawn.

| Rule | Value |
|---|---|
| Default / `direction = none` | Infini camera local. Inbound tablet `viewport` **ignored** (log; 0 apply; 0 implicit follow-on) |
| While `direction = epaper_to_infini` | Apply inbound tablet `viewport` immediately to `translate` / `scale` / drawingRegion |
| After settle while following | Infini view matches tablet region (**0** divergent viewports) |
| Infini local-nav (trackpad/mouse pan or pinch/zoom) while following | Set `direction = none` **then** drive local camera ([SRS-IN-26](../tablet-sync/srs-logic.md#srs-in-26-viewport-follow)). **0** continued tablet apply after that gesture starts |
| While `direction = infini_to_epaper` | Infini is leader: local gestures publish `viewport` **down** ([SRS-IN-21](../tablet-sync/srs-logic.md#srs-in-21-viewport-token)); do **not** apply inbound tablet `viewport` |
| Link down | Follow already `none`; local gestures still work |

`viewportOwner` / steal / 150 ms release are **withdrawn**.
