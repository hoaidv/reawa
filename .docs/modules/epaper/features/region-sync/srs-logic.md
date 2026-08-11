---
feature: region-sync
parent_req: [REQ-02]
version: 0.2.0
lifecycle: active
---

# SRS — Region sync Epaper (Logic)

Epaper side of [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md).
Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/srs-logic.md).
Ink capture: [SRS-EP-01](../local-pen-ink/srs-logic.md). Document ops:
[SRS-IN-09](../../../infini/features/vector-document/srs-data.md).

## [SRS-EP-02] Viewport map, document ops, panel refresh

### Endpoint(s)

Same session transport as Infini (JSON-lines baseline). Epaper is document-channel
**producer** for pen ink and **consumer** of viewport + remote ops.

### On viewport message (Infini → Epaper)

1. Update input→world and ink transform **immediately** (before next pen sample).
2. Enqueue region refresh (async; ghosting OK).

### On local pen ([SRS-EP-01](../local-pen-ink/srs-logic.md))

1. Ink locally with **current** map.
2. Convert samples to **world** coordinates.
3. Emit `append_ink` on the document channel with full tablet channels present
   (`x,y` + pressure/tilt/… per [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) §7).
4. Default `parentId`: document root (or active frame if product sets one later).
5. Do **not** run Smart Group enclose recognition on-device in v0 (Infini-first).

### On remote `doc_op`

- Apply idempotently by `opId` to the local materialised tree (same schema as Infini).
- Next region refresh uses updated tree ∩ current drawing region.

### On region refresh

- Rasterise **current document ∩ current drawing region**.
- Never paint a stale document with a new map (or vice versa) in the same paint pass.
- Ghosting / laggy pixels allowed; **divergent document content** for that region is not.

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

- Optional legacy `StrokeSync` / `RM_SYNC_HOST` must not own the document channel once
  ADR-0009 session is active (migrate or disable).
- Implement deferred to W4 with Infini tablet-sync.

---

## Superseded

_None._ Thickened 2026-08-11 for document-channel bind.
