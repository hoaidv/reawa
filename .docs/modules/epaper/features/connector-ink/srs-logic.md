---
feature: connector-ink
parent_req: [REQ-09]
version: 0.1.0
lifecycle: active
---

# SRS — Connector-ink (Logic)

Device rules for [REQ-09](../../prd.md#device-connectors).
Geometry: [ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md).
Dispatch: [ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md).
Op set: [SRS-EP-07](../device-document/srs-logic.md). Mirror: [SRS-IN-09](../../../infini/features/vector-document/srs-data.md).

## [SRS-EP-17] Connector recognition {#srs-ep-17-connector-recognition}

Pen-up step 4 of ADR-0022, after endpoint-ink and draw-into membership, then failed enclose.

### Guards (all must pass)

| # | Guard | Fail → |
|---|---|---|
| 1 | Open — first/last sample far relative to path length | ordinary ink |
| 2 | Path-like — low self-intersection; hull area small vs length² | ordinary ink |
| 3 | Long enough — arc ≥ `MIN_CONNECTOR_WORLD` (start 48 u, same order as enclose) | ordinary ink |
| 4 | Two bindings — first sample within `R_SNAP` of SmartGroup A **boundary ink**, last of B, **A ≠ B** | ordinary ink |
| 5 | Body mostly outside — ≤20% samples in A, ≤20% in B, ≥60% outside every box (**boundary-ink interior**, not the fitted AABB) | ordinary ink |

v1 targets **SmartGroup only**.

### Chain (UX2)

On a stroke, take the **last 3 free inks** in paint order — not the last 3 consecutive root
siblings. Skip SmartGroups, connectors, draw-into membership, and other non-ink nodes in
z-order; those do not drop older free inks from the window. Ignore free ink and connector
when asking what an endpoint **touches** (those are non-shape).

Call the window inks by role (not z-order):

| Role | Shape snap | Join |
|---|---|---|
| **B** | Exactly **one** end within `R_SNAP` of an ink-box | — |
| **C** | Exactly **one** end within `R_SNAP` of a **different** ink-box | — |
| **A** | **Neither** end snaps to a shape | Intersects or comes within `R_JOIN` **6 u** of **both** B and C |

Try in order. **No graph search** — no DFS, no longest-path, no “last N consecutive root siblings, stop at a box.” Decision: [ADR-0036](../../../../adr/ADR-0036-toolcanvas-live-overlay.md).

1. **B–A–C** when all three roles are present in the window.
2. **B–C** when two arms join each other (intersect or `R_JOIN` 6 u) with no bridge — `<box1> seg1 seg2 <box2>`. Prefer the other arm **newest** in paint order that matches the current stroke.

If both fail, UX1 on the **current** stroke only. Older free inks outside the last-3 window are ignored.

### Style pick

After the rest spine `S` exists, inflection count of `S`: ≤1 → `warpStyle: cubic` (Curve);
else `morph` (Ink). Override later does not rewrite `S`.

### Commit

`create_connector` immediately — body children, rest shape, both anchors (`kind`, `edge`,
`t`, `drawnEdgeLocal` / centre cone), `warpStyle`, reserved `terminal` slot. Device **is**
an author of this op. Log `[recog] outcome=connector`.

Visibility p95 ≤500 ms after pen-up. One-shot blink: connector + both bound nodes
([SRS-EP-19](./srs-ui.md)).

## [SRS-EP-18] Warp, anchors, delete {#srs-ep-18-connector-warp}

Canonical algorithm: [ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md) §§1–5.
Do not copy the EXP probe; reimplement the ADR.

| Rule | Value |
|---|---|
| Inputs | rest shape + two endpoint states + `warpStyle` |
| Morph | `m = 0.5·(1−cos(π·min(1, turn/90°)))`; `m=0` is a true skip (`V=U`) |
| Cubic | Hermite through endpoints with facings; handle speed = `L'` |
| `d` | absolute world units; never scaled; never re-bake rest shape |
| Live drag | re-warp each move sample; ToolCanvas damage old∪new connector AABB; CanvasLayer punches **origin box ∪ origin spine** (suppress ids are not enough — InPlaceDirty must cover the spine). 0 full-panel invalidations |
| Settle after bound-node drag | InPlaceDirty **origin ∪ live box ∪ origin and live spines**; then 1× document paint. Box-only dirty leaves origin connector and holes the new middle |
| Undo / redo of bound-node drag | Counterpart restores the box; **refresh derived warps**; InPlaceDirty **pre ∪ post box ∪ both spines**. Same hole as settle if the dirty rect is box-only — the connector is not an undo target |
| Missed frame | keep last warped pose; commit warp on pen-up |
| Delete | keep connector; missing `nodeId` resolves from last live world pose (derived cache, persisted, not an op) |
| Undo delete | same `nodeId` restored → live resolve again |

Centre clip at box boundary; keep full rest shape. Switching Edge↔Centre is creator-only.

Shared fixtures with Infini: same rest shape + endpoints + style → byte-comparable samples
(0 divergent nodes).

---

## [SRS-EP-34] Per-end endpoint styles {#srs-ep-34-end-styles}

<!-- lifecycle: active -->
<!-- campaign: not Epaper this lock — Infini / web-desktop Path A later -->

**Parent:** [REQ-13](../../prd.md#connector-ends) Path A. **Decision:** [ADR-0038](../../../../adr/ADR-0038-endpoint-ink-face-frame.md) (Epaper Path B). **Warp:** [SRS-EP-18](#srs-ep-18-connector-warp) / [ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md). **UI:** [SRS-EP-36](./srs-ui.md#srs-ep-36-endpoint-toolbar).

**Epaper this campaign: do not implement.** Closed-style chips are Infini / web-desktop.

Closed style set (Designer must not invent others): `none` · `arrow` · `arrow_empty` · `star` · `one` · `many`.

| Rule | Value |
|---|---|
| Scope | **Each end independently** on a **selected** connector |
| Op | `set_connector_end_style { connectorId, end: start\|finish, style }` |
| Latency | Style visible p95 ≤300 ms; **other** end unchanged |
| Undo | One undo reverts that end's style |
| Warp | Styles remain on the correct ends; committed geometry = last preview (0 px jump) — REQ-09 bar |
| Default | `none` at recognition unless Path B binds ink |

Does not replace recognition or warp (REQ-09). Does not steal spine ink (Path B is SRS-EP-35).

---

## [SRS-EP-35] Endpoint-ink membership {#srs-ep-35-endpoint-ink}

<!-- lifecycle: active -->

**Parent:** [REQ-13](../../prd.md#connector-ends) Path B. **Product:** [SRS-EP-74](./srs-product.md#srs-ep-74-endpoint-ink-product). **Decision:** [ADR-0038](../../../../adr/ADR-0038-endpoint-ink-face-frame.md) (supersedes [ADR-0026](../../../../adr/ADR-0026-endpoint-ink-membership.md); amends [ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md)).

Pen-up order: **this test** → draw-into membership → enclose → new connector (open only) → ordinary ink. Armed only when `recog.connector` latched at pen-down. Short ticks that the closure classifier treats as closed-ish still run this test **first**.

| Region | Steal? |
|---|---|
| ≥80% of stroke **length** in a **5 mm** (world) circle at **one** end of **one** existing connector | **Yes** — `set_endpoint_ink` appends a face-frame stroke on that `ConnectorAnchor`; `remove_node` the free Ink; 0 second connector |
| Spine, empty, mixed, two ends, two connectors | **No** |
| Connector recognition off | **No** |

Storage: `ConnectorAnchor.styleInk[]` polylines `{n, e}` in the live face frame. Paint as drawn. Bound-node transform keeps `{n, e}` and `drawnN` / `drawnE` / `drawnBoxX` / `drawnBoxY` unchanged. World paint rotates that face frame by **α** = signed angle from the stored leave (those drawn components, reconstructed as `WarpEnd.f`) to the **re-warped sample tangent** at that end, so decoration stays consistent with the spine. Rest spine **not** rebaked.

Erase ticks (keep connector): brush/area clip `styleInk` in world; object-erase drops a stroke at ≥80% length inside the lasso. Object-erase of the connector still takes decoration with it.

Wrong bind: one undo. Log `[recog] outcome=endpoint_ink end=from\|to id=<connectorId>`.

---

## [SRS-EP-38] Mid-attachment parameter t {#srs-ep-38-attachment-t}

<!-- lifecycle: active -->

**Parent:** [REQ-14](../../prd.md#connector-attachments). **Decision:** [ADR-0027](../../../../adr/ADR-0027-attachment-t-rest-spine.md). **UI:** [SRS-EP-39](./srs-ui.md#srs-ep-39-attachment-ui).

| Rule | Value |
|---|---|
| Bind | `{ nodeId, t ∈ [0,1] on rest spine S, d }` via `bind_attachment` |
| Live | Bound-box drag re-warps connector **and** re-places attachment at ≥5 Hz; 0 full-panel invalidations |
| Commit | Pose on pen-up = last preview (0 px jump) |
| Undo box move | Connector **and** attachment return to pre-move pose (±1 px @ 100% zoom) |
| Zero attachments | REQ-09 / SRS-EP-18 unchanged (0 regression) |
| Rebake | **Forbidden** |

Connector is not a spatial parent. 0 extra ops per drag frame (still `set_smart_transform` on the box).

---

## [SRS-EP-46] Manual connector and attach {#srs-ep-46-manual-connector}

<!-- lifecycle: active -->

**Parent:** [REQ-17](../../prd.md#manual-create) (Should). **Warp:** same as [REQ-09](../../prd.md#device-connectors) / SRS-EP-18. **Attach:** [SRS-EP-38](#srs-ep-38-attachment-t) must hold.

| Gesture | Result |
|---|---|
| Manual connector between two bindable nodes | `create_connector` with same rest/warp contract as recognition (creator-drawn or straight rest per place UI — Designer; geometry still ADR-0020) |
| Manual attach an existing node to a connector | `bind_attachment` with `t` from place point on **S** |

---

## Superseded
_None in this file._ Dispatch retires clauses in [SRS-EP-10](../ink-box/srs-logic.md).
REQ-13 Path A chrome was listed out of scope on [SRS-EP-19](./srs-ui.md); new parent is [SRS-EP-36](./srs-ui.md#srs-ep-36-endpoint-toolbar) ([CHL-0022](../../../../../.plan/iter-005/challenges/CHL-0022-shipped-no-device-pan.md)).
