---
feature: region-sync
parent_req: [REQ-02]
version: 0.4.0
lifecycle: active
---

# SRS — Region sync Epaper (Logic)

Epaper side of [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)
(interim wire) and [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/srs-logic.md).
Local ink: [SRS-EP-01](../local-pen-ink/srs-logic.md).

**Code SoT (2026-08-11):** `epaper/tabletcanvasitem.cpp`, `epaper/strokesync.cpp`.
Header-only `epaper/regionsync/` is **unit-tested**, not linked into the device binary.

## [SRS-EP-02] Viewport map, vector picture, panel refresh

Parent REQ: [REQ-02](../../prd.md#region-sync).

### Endpoint(s)

JSON-lines TCP to Infini (`RM_SYNC_HOST`, typically Mac USB `10.11.99.12:9877`).
Epaper **produces** `stroke_*` and **consumes** `viewport` + `doc_snapshot`.

### Digitizer → panel (always)

Independent of gut orientation — [SRS-EP-01](../local-pen-ink/srs-logic.md) Round 19:

```text
rx = raw.y * (w / h)
ry = h - raw.x * (h / w)
```

Wire `stroke_point` uses **panel** coords after this map.

### Panel → world (gut-aware)

1. Panel → frame UV via gut orientation (same rules as Infini `panelToFrameUv`).
2. UV → world via current `drawingRegion` AABB.

```text
worldX = drawingRegion.minX + u * (maxX - minX)
worldY = drawingRegion.minY + v * (maxY - minY)
```

Orientation normalize: `portrait`→`gutToLeft`, `landscape`→`gutOnTop`;
pass-through `gutOnTop|gutToLeft|gutAtBottom|gutToRight`. Default `gutToLeft`.

### Stroke width (ADR-0012)

```text
worldW = 2.5 * (0.7 + 0.3 * pressure)   // fixed at stroke begin
s_panel = panelW / drawingRegion.width
lineWidth_px = max(1, worldW * s_panel)
```

Applies to **live** `emitSegment` and **vector** `drawVectorNode`. Wire
`stroke_begin.brush.width` = world units.

### On `viewport` (Infini → Epaper)

1. Update `drawingRegion`, `orientation`, `seq` **immediately** (map before next pen).
2. Read `settle` bool → `scheduleVectorRasterize(settle)`.

### On `doc_snapshot`

1. Replace local `m_vectorNodes` from `nodes[]`.
2. Sharp rasterize immediately.

### On `region_refresh`

**Ignore** (legacy bitmap). Log once; use `doc_snapshot` + local rasterize.

### On local pen

1. Ink locally with current map + world×`s_panel` width.
2. Stream `stroke_*` (panel x/y).
3. On stroke end, append a world `path` node into local vector list (for later redraws).

### Region refresh coalesce

| Mode | Behavior |
|---|---|
| Soft (`settle=false`) | Coalesce to ≥ **250 ms** between soft rasterizes |
| Sharp (`settle=true` or `doc_snapshot`) | Immediate full redraw; antialiasing on for sharp |

Soft may look faded on e-ink; settle must sharpen.

### Paint pass

- Clear + redraw all vector nodes through `worldToPanel` (gut-aware).
- Coherent `(nodes, drawingRegion, orientation)` per paint.
- Ghosting OK; divergent content for the region after settle is not.

### Emit matrix (Qt app — shipped)

| Message | Emit? |
|---|---|
| `stroke_begin` / `stroke_point` / `stroke_end` | **yes** |
| `append_ink` / `doc_op` | **no** (library only) |
| Structure / Smart Group | **no** |

### `regionsync/` library (not device runtime)

| Piece | Role |
|---|---|
| `viewport_map.hpp` | Naive panel→region (no gut UV) |
| `doc_store.hpp` / `region_session.hpp` | `append_ink` + coalesce `PaintPass` API |
| Tests | `epaper/tests/regionsync_test.cpp` |

Future migration may wire this beside `StrokeSync`; until then SRS-EP-02 production
behavior is the Qt canvas item above.

### Errors

| Case | Behavior |
|---|---|
| Viewport missing fields | Keep last good map; log |
| Stroke send fail | Do not block local ink |
| Unknown host message | Ignore + log |

---

## Superseded

Prior “production `append_ink` + RegionSession owns map” wording is **target architecture**;
**shipped** path is documented above (0.4.0, 2026-08-11).
