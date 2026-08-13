---
feature: region-sync
parent_req: [REQ-02]
version: 0.5.0
lifecycle: active
---

# SRS — Region sync Epaper (Logic)

Epaper side of the viewport channel under
[ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) /
[ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md), with stroke paint parity from
[ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/srs-logic.md).
Local ink: [SRS-EP-01](../local-pen-ink/srs-logic.md).
Document + sync contract: [SRS-EP-07 / SRS-EP-08](../device-document/srs-logic.md).

**Code SoT (2026-08-11):** `epaper/tabletcanvasitem.cpp`, `epaper/strokesync.cpp`.
Header-only `epaper/regionsync/` is **unit-tested**, not linked into the device binary.

## [SRS-EP-02] Viewport map, vector picture, panel refresh

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. The local document is authoritative for paint;
     doc_snapshot replace-on-arrival is withdrawn. Same id, content revised. -->

Parent REQ: [REQ-02](../../prd.md#region-sync).

> **Revised 2026-08-13.** This section owns the **map**, not the picture. The picture comes from the
> device's own document ([SRS-EP-07](../device-document/srs-logic.md)); the only inbound document
> message is the handshake-gated `doc_load`, handled by
> [SRS-EP-08](../device-document/srs-logic.md). "Replace `m_vectorNodes` on every `doc_snapshot`" is
> **withdrawn** — that reflex is what let a peer overwrite the creator's work mid-session.

### Endpoint(s)

JSON-lines TCP to Infini (`RM_SYNC_HOST`, typically Mac USB `10.11.99.12:9877`).
Epaper **produces** `stroke_*` (preview) + `doc_change`, and **consumes** `viewport` + one
`doc_load` per session epoch.

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

### On `doc_load`

Delegated to [SRS-EP-08](../device-document/srs-logic.md): legal only at a session epoch boundary
and only after the change queue has drained. On acceptance the local document is replaced wholesale
and a sharp rasterize follows. **This section never mutates the document.**

An unsolicited `doc_load` mid-session is a protocol defect: reject, log, surface in the status line
([SRS-EP-05](../tool-modes/srs-ui.md)). Do not apply it.

### On `region_refresh`

**Ignore** (legacy bitmap). Log once; rasterize from the local document.

### On local pen

1. Ink locally with current map + world×`s_panel` width.
2. Stream `stroke_*` (panel x/y) as a **preview** for the desktop
   ([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) §6).
3. On stroke end, hand the stroke to [SRS-EP-07](../device-document/srs-logic.md) for ingestion into
   the local document, which is what later redraws paint from.

### Region refresh coalesce

| Mode | Behavior |
|---|---|
| Soft (`settle=false`) | Coalesce to ≥ **250 ms** between soft rasterizes |
| Sharp (`settle=true`, accepted `doc_load`, or a committed local op) | Immediate full redraw; antialiasing on for sharp |
| During a manipulation gesture | **Partial refresh only** — no full-panel invalidation ([SRS-EP-11](../ink-box/srs-logic.md)) |

Soft may look faded on e-ink; settle must sharpen.

### Paint pass

- Clear + redraw from the **local document** through `worldToPanel` (gut-aware).
- Coherent `(document, drawingRegion, orientation)` per paint.
- Ghosting OK; divergent content for the region after settle is not.
- **0 repaints sourced from an inbound peer picture** ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2).

### Emit matrix (target — ADR-0015 v1)

| Message | Emit? |
|---|---|
| `stroke_begin` / `stroke_point` / `stroke_end` | **yes** — preview only |
| `doc_change` | **yes** — one per committed op ([SRS-EP-08](../device-document/srs-logic.md)) |
| `hello` / `queue_empty` / `load_ack` | **yes** — handshake |
| `stroke_begin.intent` / `tool_intent` | **no** — retired with [SRS-IN-13](../../../infini/features/tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport) |

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
