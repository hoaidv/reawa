---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.3.0
lifecycle: active
---

# SRS — Tablet sync Infini (Logic)

Binds Infini to the shared session in [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)
and stroke paint parity in [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Document tree + ops: [vector-document SRS-IN-04/09](../vector-document/srs-logic.md) ·
[ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md).

## [SRS-IN-07] Session roles and channel binding

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

### Endpoint(s)

Local network session to RM2 (USB Ethernet). Framing baseline: **JSON-lines** messages
(EXP-0001 spike). Auth: none in v0 (trusted local link).

### Channels

| Channel | Owner (writer) | Consumer | Priority |
|---|---|---|---|
| **Viewport** | Infini | Epaper | High — apply before next pen sample |
| **Document** | Both (see emit matrix) | Both | Normal — ordered op-log |

### Tablet drawing frame (CSS) and `drawingRegion` (world)

Infini maintains a **tablet drawing frame**: an axis-aligned rectangle in **CSS window
pixels** with the Epaper panel aspect ratio (RM2 portrait/landscape as product configures).
Default placement: **centered** in the canvas host (resize keeps aspect; may letterbox).

| Concept | Space | Meaning |
|---|---|---|
| Tablet drawing frame | CSS px | What the marker outlines; maps 1:1 to Epaper panel |
| `drawingRegion` | World AABB | Inverse-map of that frame under current viewport |

```text
drawingRegion = screenToWorld(frame.min) … screenToWorld(frame.max)
  where screenToWorld(p) = p / scale - translate   // [SRS-IN-01]
```

`drawingRegion` in the viewport message **must** be this frame’s world AABB — **not**
necessarily the full CSS window. Full-window AABB is only correct if the frame fills the
host (tests may use that).

### Drawing-region marker (gesture affordance)

| State | Marker |
|---|---|
| Idle (`ui.gesturing = false`) | **Not visible** (no permanent chrome frame) |
| Pan/zoom active (`ui.gesturing = true`) | **Visible** outline of the tablet drawing frame |
| Gesture end | Hide after settle (≤ 150 ms fade allowed; must not remain as chrome) |

Marker is a **sync affordance** only (REQ-03 Needs design: no). It tracks the CSS frame
rect; it does not invent a second region.

### Viewport message (Infini → Epaper)

| Field | Required | Meaning |
|---|---|---|
| `type` | yes | `viewport` |
| `translate` | yes | `{ x, y }` world — Infini canvas translate |
| `scale` | yes | uniform > 0 — Infini canvas scale |
| `drawingRegion` | yes | World AABB of tablet drawing frame |
| `seq` | yes | monotonic |

### Viewport publish coalesce

| Rule | Value |
|---|---|
| During continuous pan/zoom | Coalesce outbound viewport to **≤ 30 Hz** (or one emit per animation frame, whichever is lower); **latest** translate/scale/region wins |
| On gesture end | **Flush** the final viewport within 1 frame (must not drop the settle pose) |
| Map freshness | Coalesce must not prevent Epaper from meeting [SRS-IN-08](./srs-quality.md) map-apply budget once a message is sent |

Infini still owns translate + scale; Epaper never pans/zooms the session viewport.

### Document op envelope (either direction)

Matches [SRS-IN-09](../vector-document/srs-data.md):

```text
{ opId, type, payload, ts?, source: "epaper"|"infini" }
```

Apply is **idempotent** on `opId`. Peers maintain the same materialised tree.

### Emit matrix (v0 sync wave)

| Op `type` | Epaper may emit | Infini may emit | Notes |
|---|---|---|---|
| `append_ink` | **yes** (primary) | later (desktop ink) | Samples in **world** space after current viewport map; width in world units ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)) |
| `insert_node` / `create_*` structure | no (v0) | yes (when desktop edits) | Groups, frames, text, primitives, connectors |
| `create_smart_group` / enclose | no (v0) | **yes** (Infini-first pilot) | Epaper keeps emitting raw ink; Infini may promote |
| `set_smart_transform` / `set_ink_scale_mode` | no | yes | |
| `reparent` / `remove_node` | no | yes | |
| Snapshot / hello | optional | optional | Reconnect catch-up TBD |

**v0 editing rule:** While Epaper is the active pen editor, Infini is **viewer + viewport
owner** for ink capture — it applies incoming `append_ink` and does not fight Epaper on the
same stroke. Smart Group / structure edits on Infini are allowed when not racing an in-flight
stroke (product: pause or queue).

### Stroke paint (Infini)

Per [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md):

- Ink `strokeWidth` is **world units**.
- Canvas paint: `lineWidth_css = strokeWidth_world * viewport.scale` (pressure curve applied in world space first).
- Zoom in → thicker CSS strokes; zoom out → thinner. Synced ink must not use screen-constant width.

### Response fields that drive UI

| Field | Required | Drives UI |
|---|---|---|
| `session.connected` | yes | optional status (chrome later) |
| `doc` tree | yes | WorldLayer projection ([SRS-IN-01](../infinity-canvas/srs-logic.md)) |
| `viewport` | yes | canvas transform |
| `ui.gesturing` | yes | marker visibility |
| `tabletFrame` CSS rect | yes when session active | marker geometry |

### Closed ids (message types)

| id | Channel | Direction |
|---|---|---|
| `viewport` | viewport | Infini → Epaper |
| `doc_op` | document | bidirectional |
| `doc_ack` | document | optional ack of `opId` |
| `hello` / `snapshot` | document | optional reconnect |

### Errors / partial failure

| Case | Behavior |
|---|---|
| Duplicate `opId` | Ignore (idempotent) |
| Unknown op type | Log; do not crash; optional NACK |
| Gap in seq / missing op | Request snapshot or mark session degraded (CHL if product needs hard fail) |
| Apply fails validation (e.g. bad parentId) | Reject op; keep prior tree; surface debug |

### Other logic

- Render path: apply doc ops → flatten tree (incl. SmartGroup transforms) → spatial cull → paint with world-width × scale.
- Live path uses ADR-0009 session (`TabletSession`); legacy EXP StrokeSync must not define the drawing-region map once the session is connected.

---

## Superseded

_None._ W5 thicken 2026-08-11: marker, tablet frame / `drawingRegion`, publish coalesce, ADR-0012 paint.
