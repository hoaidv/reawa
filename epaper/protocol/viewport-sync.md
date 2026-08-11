# Viewport + stroke sync protocol (shipped)

Bidirectional JSON-lines over TCP between Infini (Electron, listen `:9877`) and
Epaper (`RM_SYNC_HOST`, typically `10.11.99.12`).

Status: **code-truth** 2026-08-11. Local ink does not depend on Infini
([SRS-EP-01](../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md)).

Product SRS: [SRS-IN-07](../../.docs/modules/infini/features/tablet-sync/srs-logic.md) ·
[SRS-EP-02](../../.docs/modules/epaper/features/region-sync/srs-logic.md).

## Coordinate spaces

| Space | Notes |
|---|---|
| **World** | Infini canvas; Y down; stroke widths in world units |
| **Tablet CSS frame** | Max-fit centered rect in Infini window (gut aspect) |
| **drawingRegion** | World AABB of that frame |
| **Panel** | RM2 framebuffer after Round 19 digitizer map |
| **Frame UV** | Panel → [0,1]² via gut orientation |

## Messages

### `viewport` (Infini → Epaper)

```json
{
  "type": "viewport",
  "seq": 42,
  "translate": { "x": -120, "y": 40 },
  "scale": 1.5,
  "drawingRegion": { "minX": 0, "minY": 0, "maxX": 800, "maxY": 1066 },
  "orientation": "gutToLeft",
  "settle": true
}
```

`settle` is set on flush / gesture end. Orientation: `gutOnTop` | `gutToLeft` |
`gutAtBottom` | `gutToRight`.

### `doc_snapshot` (Infini → Epaper)

```json
{
  "type": "doc_snapshot",
  "nodes": [
    { "kind": "path", "id": "p1", "strokeWidth": 2.5, "points": [{ "x": 1, "y": 2 }] },
    { "kind": "ellipse", "id": "e1", "strokeWidth": 2.5, "cx": 0, "cy": 0, "rx": 40, "ry": 40 }
  ]
}
```

One-shot / rare. Not a full tree-of-vectors dump.

### `stroke_*` (Epaper → Infini)

```json
{"type":"stroke_begin","id":"s-1","brush":{"width":2.5},"cw":1404,"ch":1872}
{"type":"stroke_point","id":"s-1","x":702,"y":936,"p":0.82}
{"type":"stroke_end","id":"s-1"}
```

`brush.width` = **world** units. `x,y` = **panel** pixels after digitizer map.

### Ignored / not sent

| Type | Status |
|---|---|
| `region_refresh` | Legacy PNG — Epaper ignores; Infini must not send |
| `doc_op` / `append_ink` | Library / future ADR-0009 path |
| `stroke_batch` | Removed — replaced by `doc_snapshot` + local rasterize |

## Re-render rules

1. **Pan/zoom Infini** — coalesce `viewport` ≤30 Hz; settle flush with `settle:true`.
2. **Draw on RM** — local ink immediately; stream `stroke_*`; Infini appends WorldLayer paths.
3. **Brush size** — world units; panel/CSS thickness = world × current scale (ADR-0012).
4. **Picture on RM** — rasterize `doc_snapshot` nodes ∩ `drawingRegion` locally; soft coalesce ≥250 ms; sharp on settle.
