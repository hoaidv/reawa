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

Pen-up step 3 of ADR-0022, after failed-enclose→membership.

### Guards (all must pass)

| # | Guard | Fail → |
|---|---|---|
| 1 | Open — first/last sample far relative to path length | ordinary ink |
| 2 | Path-like — low self-intersection; hull area small vs length² | ordinary ink |
| 3 | Long enough — arc ≥ `MIN_CONNECTOR_WORLD` (start 48 u, same order as enclose) | ordinary ink |
| 4 | Two bindings — first sample within `R_SNAP` of SmartGroup A bounds, last of B, **A ≠ B** | ordinary ink |
| 5 | Body mostly outside — ≤20% samples in A, ≤20% in B, ≥60% outside every box | ordinary ink |

v1 targets **SmartGroup only**.

### Chain (UX2)

On a stroke that binds a second node, walk back through the last **N≈8** free top-level inks
for endpoint-adjacent (`R_JOIN`) and tangent-continuous (turn ≤ ~60°) links, either draw
direction. If the far end binds a different SmartGroup, merge the chain into **one**
`create_connector` (body = strokes in draw order) with **one** undo entry. No pending node.

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
| Live drag | re-warp each move sample; damage old∪new AABB; 0 full-panel invalidations |
| Missed frame | keep last warped pose; commit warp on pen-up |
| Delete | keep connector; missing `nodeId` resolves from last live world pose (derived cache, persisted, not an op) |
| Undo delete | same `nodeId` restored → live resolve again |

Centre clip at box boundary; keep full rest shape. Switching Edge↔Centre is creator-only.

Shared fixtures with Infini: same rest shape + endpoints + style → byte-comparable samples
(0 divergent nodes).

---

## Superseded
_None in this file._ Dispatch retires clauses in [SRS-EP-10](../ink-box/srs-logic.md).
