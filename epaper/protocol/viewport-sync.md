# Viewport + stroke sync protocol (spike → product draft)

Bidirectional sync between a macOS infinity canvas viewer and the on-device
Epaper drawing app over TCP (USB `10.11.99.1` or Wi‑Fi).

Status: draft from [EXP-0001](../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md).
Local ink ([SRS-EP-01](../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md)) does not depend on this file.

## Coordinate spaces

| Space | Origin | Notes |
|---|---|---|
| **Canvas world** | Top-left, Y down | Infinite 2D; all strokes stored here |
| **Visible frame** | Sub-rect of canvas world | macOS window content rect after pan/zoom |
| **Drawing frame** | Fixed center of visible frame (UI coords) | Maps 1:1 to RM screen pixels; only this region syncs to RM |
| **RM device** | Digitizer / mapped scene | Pen samples; Epaper maps to panel scene per SRS-EP-01 |

Transform on macOS pan/zoom: update `visible_origin` + `zoom`; drawing frame stays centered in window; world rect under drawing frame changes → sent to RM as new viewport.

## Messages (JSON lines, newline-delimited)

### `viewport` (macOS → RM)

```json
{
  "type": "viewport",
  "seq": 42,
  "world": { "x": -1200, "y": -800, "w": 2400, "h": 1600 },
  "drawing_frame": { "cx": 1200, "cy": 800, "w": 1404, "h": 1052 },
  "zoom": 1.25,
  "brush_scale": 1.25
}
```

### `stroke_begin` / `stroke_point` / `stroke_end` (RM → macOS)

```json
{"type":"stroke_begin","id":"s-1","brush":{"width":2.0,"opacity":1.0}}
{"type":"stroke_point","id":"s-1","x":10234,"y":5678,"p":0.82,"t":1710000000123}
{"type":"stroke_end","id":"s-1"}
```

### `stroke_batch` (macOS → RM, on viewport change)

After pan/zoom, macOS sends clipped stroke segments for the new drawing-frame world rect so RM can full-frame e-paper refresh.

## Re-render rules

1. **Pan/zoom on macOS** — recompute world rect under drawing frame; send `viewport`; RM clears and repaints from `stroke_batch`.
2. **Draw on RM** — local ink immediately; stream points to macOS; macOS appends to world model.
3. **Brush size** — store strokes in world units at zoom=1; scale at render time via `brush_scale`.
