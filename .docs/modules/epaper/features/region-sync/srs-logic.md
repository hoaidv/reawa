---
feature: region-sync
parent_req: [REQ-02, REQ-10, REQ-19]
version: 0.6.0
lifecycle: active
---

# SRS — Region sync Epaper (Logic)

Epaper side of the viewport channel under
[ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) /
[ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md), as amended by
[ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md)
(independent cameras; follow-gated viewport). Stroke paint parity from
[ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/srs-logic.md).
Local ink: [SRS-EP-01](../local-pen-ink/srs-logic.md).
Document + sync contract: [SRS-EP-07 / SRS-EP-08](../device-document/srs-logic.md).
Follow anatomy: [domain/viewport-follow](../../../../domain/viewport-follow.md).

**Code SoT (2026-08-11):** `epaper/tabletcanvasitem.cpp`, `epaper/strokesync.cpp`.
Header-only `epaper/regionsync/` is **unit-tested**, not linked into the device binary.

## [SRS-EP-02] Viewport map, vector picture, panel refresh

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. The local document is authoritative for paint;
     doc_snapshot replace-on-arrival is withdrawn. Same id, content revised. -->
<!-- revised: 2026-08-20 — ADR-0029. Apply Infini viewport only while Epaper follow is on.
     Same id; follow toggle is [SRS-EP-49], not a parent of this section. -->

Parent REQ: [REQ-02](../../prd.md#region-sync). **Gate (not parent):** [REQ-19](../../prd.md#viewport-follow) / [SRS-EP-49](#srs-ep-49-viewport-follow).

> **Revised 2026-08-13.** This section owns the **map**, not the picture. The picture comes from the
> device's own document ([SRS-EP-07](../device-document/srs-logic.md)); the only inbound document
> message is the handshake-gated `doc_load`, handled by
> [SRS-EP-08](../device-document/srs-logic.md). "Replace `m_vectorNodes` on every `doc_snapshot`" is
> **withdrawn** — that reflex is what let a peer overwrite the creator's work mid-session.

### Endpoint(s)

JSON-lines TCP to Infini (`RM_SYNC_HOST`, typically Mac USB `10.11.99.12:9877`).
Epaper **produces** `stroke_*` (preview) + `doc_change` + `viewport` **only while Infini follow is on**,
and **consumes** `viewport` **only while Epaper follow is on** + one `doc_load` per session epoch
+ `viewport_follow`. Default: independent cameras — **0** viewport either way.

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

**Only while Epaper follow is on** (`direction = infini_to_epaper`). Otherwise: ignore + log; **do not** apply; **do not** treat arrival as implicit follow-on ([ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md)).

While following:

1. Update `drawingRegion`, `orientation`, `seq` **immediately** (map before next pen).
2. Read `settle` bool → `scheduleVectorRasterize(settle)`.

Local camera is the default map. Infini pan/zoom does not change it while follow is off.

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
| `viewport` (up) | **only** while Infini follow is on ([SRS-EP-24](#srs-ep-24-two-finger-viewport)) |
| `viewport_follow` | **yes** — session enum ([SRS-EP-49](#srs-ep-49-viewport-follow)); **0** document |
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

## [SRS-EP-24] Two-finger pan/zoom and viewport publish {#srs-ep-24-two-finger-viewport}

<!-- lifecycle: active -->
<!-- revised: 2026-08-20 — ADR-0029. Local Must; publish only if Infini follow is on. BRD-07 ship gate lifted. -->

**Parent:** [REQ-10](../../prd.md#hand-touch) (two-finger half; [REQ-16](../../prd.md#device-pan-zoom) retired). **Decision:** [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md). **Links (not parents):** [SRS-EP-02](#srs-ep-02) inbound Infini map apply, [SRS-EP-49](#srs-ep-49-viewport-follow) follow enum, [SRS-EP-08](../device-document/srs-logic.md). Do **not** parent Infini [REQ-06](../../../infini/prd.md#viewport-follow) here.

Local two-finger pan/pinch is **Must**. Last-writer token is withdrawn.

### Gesture

| Rule | Value |
|---|---|
| Fingers | **Two** on empty canvas (or not claiming a one-finger box-move) |
| One-finger box-move or resize in flight | Two-finger **does not run**; 0 pan from the move/resize ([SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger)) |
| Pan | Translate `drawingRegion` / equivalent `translate` |
| Pinch | Uniform `scale` only (no rotate/skew) |
| Local map | Apply **immediately** so the next pen sample is correct (p95 ≤100 ms map apply — [SRS-EP-26](./srs-quality.md#srs-ep-26-two-finger-quality)) |
| If Epaper follow is on at gesture start | Set `direction = none` **then** drive the local camera ([SRS-EP-49](#srs-ep-49-viewport-follow) follower local-nav) |
| Publish | `viewport` **up** with `source: epaper` **only if** Infini follow is on (`direction = epaper_to_infini`). Otherwise **0** viewport up |
| Flush | `settle: true` on two-finger end when publishing |
| Link down | Local map still updates; follow is already `none`; **0** publish |

Does not change digitizer→panel Round 19 ([SRS-EP-01](../local-pen-ink/srs-logic.md)). Does not paint from a peer picture.

### UI-driving fields

| Field | Drives |
|---|---|
| `touch.fingerCount` | Must be 2 |
| `follow.direction` | Publish iff `epaper_to_infini`; local-nav iff was `infini_to_epaper` |
| `viewport.{translate,scale,drawingRegion,settle}` | Local map; Infini apply only while Infini following |

---

## [SRS-EP-49] Viewport-follow Infini {#srs-ep-49-viewport-follow}

<!-- lifecycle: active -->

**Parent:** [REQ-19](../../prd.md#viewport-follow). **Decision:** [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md). **Anatomy:** [domain/viewport-follow](../../../../domain/viewport-follow.md). **Map apply (not parent):** [SRS-EP-02](#srs-ep-02). **Peer:** [SRS-IN-26](../../../infini/features/tablet-sync/srs-logic.md#srs-in-26-viewport-follow). **Do not parent this on SRS-EP-02 / SRS-EP-24 / SRS-EP-05.**

Follow is a **choice**. Default `none`. Not a ToolChip exclusive tool, recognizer, or hand-tool tile.

### Session enum (closed)

| `direction` | Epaper camera | Viewport on wire |
|---|---|---|
| `none` (default) | Local | **0** either way |
| `infini_to_epaper` | Apply Infini `viewport` ([SRS-EP-02](#srs-ep-02)) | Infini → Epaper only |
| `epaper_to_infini` | Local; Infini is the follower | Epaper → Infini only (from local nav, [SRS-EP-24](#srs-ep-24-two-finger-viewport) / [SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger) past threshold) |

### Rules

| Trigger | Effect |
|---|---|
| Creator enables Epaper follow (session live, was `none`) | Set `infini_to_epaper`; emit `viewport_follow`; apply Infini’s current viewport (p95 map ≤100 ms); Infini follow stays off |
| Creator enables Epaper follow while Infini follow is on | Set `infini_to_epaper`; Infini follow **off** (0 dual-on; peer toggle p95 ≤300 ms); start applying Infini viewport |
| Creator disables Epaper follow | Set `none`; stop applying inbound `viewport` |
| Follower local-nav (one-finger empty pan **past** 10 mm, or two-finger pan/pinch) while `infini_to_epaper` | Set `none` **then** local camera; 0 continued Infini apply after that gesture starts |
| Box pick / move / resize | **Does not** change follow |
| Connection lost / no session | Force `none` before the next gesture. Reconnect does **not** restore |
| Inbound `viewport` while not `infini_to_epaper` | Ignore + log; 0 apply; 0 implicit on |
| Inbound `viewport_follow` | Adopt `direction` (latest `seq`); update toggle |

`viewport_follow` is session, not document (**0** `doc_*` from this feature).

### UI-driving fields

| Field | Drives |
|---|---|
| `follow.direction` | Toggle state ([SRS-EP-50](./srs-ui.md#srs-ep-50-follow-toggle)) |
| `session.connected` | Toggle unavailable / forced off when false |

---

## Superseded

Prior “production `append_ink` + RegionSession owns map” wording is **target architecture**;
**shipped** path is documented above (0.4.0, 2026-08-11).
