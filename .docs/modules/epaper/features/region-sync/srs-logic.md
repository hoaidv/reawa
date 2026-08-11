---
feature: region-sync
parent_req: [REQ-02]
version: 0.3.0
lifecycle: active
---

# SRS — Region sync Epaper (Logic)

Epaper side of [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) and
[ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/srs-logic.md).
Ink capture: [SRS-EP-01](../local-pen-ink/srs-logic.md). Document ops:
[SRS-IN-09](../../../infini/features/vector-document/srs-data.md).

## [SRS-EP-02] Viewport map, document ops, panel refresh

Parent REQ: [REQ-02](../../prd.md#region-sync).

### Endpoint(s)

Same session transport as Infini (JSON-lines baseline). Epaper is document-channel
**producer** for pen ink and **consumer** of viewport + remote ops.

### Panel → world map

Given current viewport map (`translate`, `scale`, `drawingRegion`) and panel size
`(panelW, panelH)` in device pixels:

```text
u = localX / panelW          // 0…1 across panel
v = localY / panelH
worldX = drawingRegion.minX + u * (drawingRegion.maxX - drawingRegion.minX)
worldY = drawingRegion.minY + v * (drawingRegion.maxY - drawingRegion.minY)
```

Do **not** treat raw panel pixels as Infini CSS screen coordinates unless the product
explicitly places the tablet frame 1:1 with a CSS origin (tests may). Production path uses
**normalized panel → `drawingRegion`**.

Uniform world→panel scale for paint:

```text
s_panel = panelW / (drawingRegion.maxX - drawingRegion.minX)   // prefer X; aspect-locked frame
lineWidth_px = strokeWidth_world * s_panel
```

### On viewport message (Infini → Epaper)

1. Update input→world and ink transform **immediately** (before next pen sample) — **no
   coalesce on the map**.
2. Mark region refresh **pending** with the latest `(doc snapshot identity, viewport seq)`.
3. Schedule / coalesce panel refresh per [SRS-EP-03](./srs-quality.md) (async; ghosting OK).

### On local pen ([SRS-EP-01](../local-pen-ink/srs-logic.md))

1. Ink locally with **current** map (and local paint using world width × `s_panel`).
2. Convert samples to **world** coordinates via panel→`drawingRegion`.
3. Emit `append_ink` on the document channel with full tablet channels present
   (`x,y` + pressure/tilt/… per [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) §7).
4. Store / emit stroke width in **world units** ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)).
5. Default `parentId`: document root (or active frame if product sets one later).
6. Do **not** run Smart Group enclose recognition on-device in v0 (Infini-first).

### On remote `doc_op`

- Apply idempotently by `opId` to the local materialised tree (same schema as Infini).
- Mark refresh pending; next coalesced refresh uses updated tree ∩ current drawing region.

### On region refresh

- Rasterise **current document ∩ current drawingRegion** (same-picture rule).
- Never paint a stale document with a new map (or vice versa) in the same paint pass —
  take one coherent `(tree revision, viewport seq)` pair per paint.
- Ghosting / laggy pixels allowed; **divergent document content** for that region is not.
- Stroke paint uses `lineWidth_px = strokeWidth_world * s_panel` so zoom/pan that shrinks
  world extent in the region thickens strokes on panel (parity with Infini).

### Emit matrix (Epaper v0)

| Op | Emit? |
|---|---|
| `append_ink` | **yes** |
| Structure / Smart Group / text / connectors | **no** (Infini or later) |

### Errors

| Case | Behavior |
|---|---|
| Viewport missing fields | Keep last good map; log |
| `append_ink` send fail | Retry/backoff; do not block local ink hot path |
| Unknown remote op | Ignore + log |

### Other logic

- Optional legacy `StrokeSync` / `RM_SYNC_HOST` must not own the document channel or the
  drawing-region map once ADR-0009 session is active (migrate or disable).
- W5 wires Qt `RegionSession` / canvas item to this contract (IN-011 / EP-002).

---

## Superseded

_None._ W5 thicken 2026-08-11: panel→region map, e-ink coalesce, ADR-0012 width.
