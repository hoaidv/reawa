---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.4.0
lifecycle: active
---

# SRS — Tablet sync Infini (Logic)

Binds Infini to the shared session in [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)
(interim wire: see ADR amendments) and stroke paint parity in
[ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Tree library: [vector-document](../vector-document/srs-logic.md).

**Code SoT (2026-08-11):** `infini/src/session/TabletSession.ts`,
`infini/src/canvas/CanvasStage.tsx`, `infini/src/canvas/Viewport.ts`,
`infini/electron/main.cjs` (TCP `:9877`).

## [SRS-IN-07] Session roles and channel binding

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

### Endpoint(s)

Local network session to RM2 (USB Ethernet). Framing: **JSON-lines** over TCP.
Auth: none in v0 (trusted local link). Default listen: `0.0.0.0:9877`.

### Channels (shipped)

| Channel | Owner (writer) | Consumer | Priority |
|---|---|---|---|
| **Viewport** | Infini | Epaper | High — apply before next pen sample |
| **Document snapshot** | Infini | Epaper | One-shot / rare — WorldLayer vectors |
| **Stroke stream** | Epaper | Infini | Normal — panel samples → world paths |

Target ADR-0009 **op-log** (`doc_op` / `append_ink`) exists in session types +
`VectorDocument` unit tests; **not** on the live CanvasStage ↔ RM path.

### Tablet drawing frame (CSS) and `drawingRegion` (world)

| Concept | Space | Meaning |
|---|---|---|
| Tablet drawing frame | CSS px | Max-fit centered rect matching panel aspect for current gut pose |
| `drawingRegion` | World AABB | `frameWorldAabb(frame, viewport)` |

Panel aspect: RM2 native `1404×1872`. Tall guts → portrait aspect; wide guts → landscape aspect.

### Orientation (four gut poses)

| Id | Frame | invertX | invertY | Notes |
|---|---|---|---|---|
| `gutToLeft` | tall | false | false | **Default** — verified vertical |
| `gutOnTop` | wide | false | false | Landscape unwrap + L/R u-flip |
| `gutAtBottom` | wide | true | true | |
| `gutToRight` | tall | true | true | |

Legacy aliases on Epaper ingest: `portrait`→`gutToLeft`, `landscape`→`gutOnTop`.

`panelToFrameUv` maps post-digitizer **panel** coords → frame UV for RM→Infini strokes.

### Drawing-region marker

| State | Marker |
|---|---|
| Idle | Not visible |
| Pan/zoom active | Visible outline of CSS frame |
| Gesture end | Hide after **100 ms** settle debounce (no fade required) |

### Viewport message (Infini → Epaper)

| Field | Required | Meaning |
|---|---|---|
| `type` | yes | `viewport` |
| `translate` | yes | `{ x, y }` |
| `scale` | yes | uniform > 0 |
| `drawingRegion` | yes | World AABB of tablet frame |
| `seq` | yes | monotonic |
| `orientation` | yes (shipped) | gut id string |
| `settle` | on flush | `true` when gesture settle / force flush |

### Viewport publish coalesce

| Rule | Value |
|---|---|
| Continuous pan/zoom | ≤ **30 Hz**; latest wins |
| Gesture end / force | `flushViewport` — bypass rate limit; `settle: true` |
| Wheel settle | ~100 ms debounce then flush |

### Document snapshot (Infini → Epaper)

```text
{ "type": "doc_snapshot", "nodes": [ WorldLayerNode, ... ] }
```

Node kinds from live WorldLayer: `line` | `rect` | `ellipse` | `path` + `id` +
`strokeWidth` (world) + geometry fields. **Not** a full ADR-0010 tree dump.

Sent on: RM connect, first viewport publish, orientation change (and whenever product
forces a resync). Pan/zoom after that is viewport-only; Epaper re-rasterizes locally.

### Stroke stream (Epaper → Infini)

| Message | Fields |
|---|---|
| `stroke_begin` | `id`, `brush.width` (**world** units), optional `cw`/`ch` panel size |
| `stroke_point` | `id`, `x`, `y` (**panel** after Round 19 map), optional `p` |
| `stroke_end` | `id` |

Infini maps panel→UV→`drawingRegion` world and appends WorldLayer `path` primitives.
Does **not** call `VectorDocument.append_ink` on the live path today.

### Stroke paint (Infini)

Per [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md):

- Ink / path `strokeWidth` is **world units**.
- Canvas: CSS thickness ≈ `strokeWidth_world * viewport.scale`.
- Zoom in → thicker; zoom out → thinner.

### Closed ids (message types) — shipped

| id | Channel | Direction |
|---|---|---|
| `viewport` | viewport | Infini → Epaper |
| `doc_snapshot` | document picture | Infini → Epaper |
| `stroke_begin` / `stroke_point` / `stroke_end` | stroke | Epaper → Infini |

### Closed ids — library / future (not live wire)

| id | Notes |
|---|---|
| `doc_op` | Typed + unit-tested; UI does not emit |
| `doc_ack` / `hello` | TBD reconnect protocol |
| `region_refresh` | Legacy PNG — **do not send**; Epaper ignores |

### Errors / partial failure

| Case | Behavior |
|---|---|
| No native bridge | Browser-only; no RM sync |
| Bad JSON line | Log; continue |
| Unknown host→RM type | Epaper ignores |
| RM disconnect | Status update; resend snapshot on reconnect |

### Other logic

- Live paint SoT: `InfiniDocument` WorldLayer (demo primitives + RM paths).
- `TabletSession` may hold a `VectorDocument` for future op sync; structure ops are not
  on the RM wire yet.

---

## Superseded

W5 wording that required live `doc_op` / `append_ink` / PNG `region_refresh` as the
production path is **superseded** by this 0.4.0 code-truth rewrite (2026-08-11). Target
op-log remains ADR-0009 long-term.
