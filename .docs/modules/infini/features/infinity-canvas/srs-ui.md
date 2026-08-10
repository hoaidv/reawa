---
feature: infinity-canvas
parent_req: [REQ-01]
version: 0.1.0
lifecycle: active
needs_design: true
---

# SRS — Infinity canvas (UI)

## [SRS-IN-02] Canvas chrome and gestures

### Surfaces

| Surface | Purpose |
|---|---|
| `scene.canvas` | Full-window (or main pane) infinity canvas |
| `chrome.status` (optional v0) | Zoom % readout |

### States matrix

| State id | When | UI |
|---|---|---|
| `canvas.empty` | No document vectors | Empty world; gestures still work |
| `canvas.populated` | ≥1 primitive/stroke | Content under transform |
| `canvas.gesturing` | Pan/zoom in progress | Continuous transform; no modal chrome |
| `canvas.resized` | Window resize | Recompute view; keep world anchor stable (top-left or center — pick in design) |

### Gesture map

| Input | Action |
|---|---|
| Trackpad two-finger pan | Pan |
| Mouse drag on canvas background | Pan |
| Wheel | Pan (axis per platform default) |
| Modifier + wheel | Zoom |
| Trackpad pinch | Zoom |

### Design notes

- No cards in the hero canvas; canvas is the composition.
- Alternate states above need Spec coverage from `/designer`.
