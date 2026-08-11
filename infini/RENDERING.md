# Infini canvas rendering

Sibling notes to [epaper/RENDERING.md](../epaper/RENDERING.md). Desktop infinity
canvas for [REQ-01](../.docs/modules/infini/prd.md#infinity-canvas).

## Lessons from ml-mindmap (and deliberate breaks)

Reference: `/Users/hoaidv/Project/ml-mindmap` — React + Canvas2D + MobX.

| Topic | ml-mindmap | Infini |
|---|---|---|
| Transform | `screen = world·s + t_screen` | SRS: `screen = (world + t_world)·s` |
| Paint set | **All** `doc.getElements()` every frame | Spatial query of visible AABB |
| Gestures | Wheel pan + ctrl/meta zoom only | + mouse-drag pan; same pinch→ctrl+wheel |
| Scale | Fine for dozens of shapes | Targets 10⁴–10⁵ dense polylines |

### Why full redraw fails at ink scale

Handwriting is mostly **root-level dense polylines**. Painting N every pan tick
costs O(N). Zoom-out makes *more* of them visible → worse. Zoom-in naturally
culls → better.

### Coping strategy (implemented)

1. **Viewport cull** via flat list (N ≤ 256) or **quadtree** (larger).
2. **Tile LOD** when `scale < 0.35` (`TILE_LOD_SCALE`): rasterize world tiles once,
   blit scaled bitmaps. Individual pick disabled at that LOD
   (`allowIndividualInteraction`).
3. When documents gain grouping: recurse the stacking tree, but **each sibling
   list** still needs the same spatial query — the index is per-level.

Thresholds are heuristics; tune with STORY-IN-005 (`?trace=1` frame counter).

## Run

```bash
cd infini
npm install
npm test
npm run electron:dev   # or: npm run dev  (browser)
```

## Traceability

| Symbol | SRS |
|---|---|
| `Viewport.ts` | SRS-IN-01 |
| `SpatialIndex.ts` / `Document.ts` / `CanvasRenderer.ts` | SRS-IN-01 |
| `CanvasStage.tsx` gestures | SRS-IN-02 |
| `App.tsx` shell | SRS-IN-01 / UI Spec WindowFrame |
| frame `?trace=1` | SRS-IN-03 |
