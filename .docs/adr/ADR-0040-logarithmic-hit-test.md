---
id: ADR-0040
title: Device logarithmic hit-test spatial index
status: proposed
date: 2026-09-05
deciders: [architect, pm]
supersedes: null
source: TRACK-005 / human 2026-09-05 hit-test complexity / [REQ-04] [REQ-06] / [SRS-EP-78] [SRS-EP-79]
---

# ADR-0040 — Device logarithmic hit-test spatial index

## Context

Every document-geometry query on Epaper still **walks the tree linearly**: tap
([SRS-EP-77](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)),
marquee / freeform ([SRS-EP-11](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation)),
draw-into membership, enclose capture, move-commit reparent (≥80% natural area),
and object-erase candidate collection. Nested boxes ([ADR-0039](./ADR-0039-nested-ink-box-rendering.md))
make the walk deeper, not smaller. The existing comfort bar is **p95 ≤100 ms**
([SRS-EP-14](../modules/epaper/features/ink-box/srs-quality.md),
[SRS-EP-13](../modules/epaper/features/device-document/srs-quality.md) 500-node fixture) —
that is latency, not complexity. Human (2026-09-05) asked for **logarithmic** query cost.

Quality stakes, in priority order:

1. **Ink latency** — index rebuild / query must not sit on the pen-down → pixel path
   ([SRS-EP-01](../modules/epaper/features/local-pen-ink/srs-logic.md) p95 ≤30 ms).
2. **Gesture truth** — product hit rules stay exact (children before ancestors; marquee
   top-level; 80% sample / area / length tests unchanged). Approximating 80% from AABBs
   would be a product trade-off — file a `CHL-*`, do not do it here.
3. **Query scaling** — [SRS-EP-78](../modules/epaper/features/device-document/srs-quality.md#srs-ep-78-log-hit-test)
   (O(log n + k) probes vs n, 500-node and 5k-node fixtures).

[ADR-0010](./ADR-0010-tree-of-vectors.md) §4 already names a spatial index for
**render/cull flatten** of drawable leaves. Infini implements that as a **quadtree over
paint primitives**. Device hit-test is still a linear walk. Those are different keys,
different invalidation, different hosts.

## Decision

1. **One R-tree (STR bulk-load) on the device document**, not a family of indexes.
   All geometry queries in [SRS-EP-79](../modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries)
   are functions over that one tree. Payload per leaf: `{id, kind, topLevelPickable,
   isSmartGroup, isLegalParent, paintRank, depth, worldAabb}`.

2. **What is indexed: composed world AABB after [ADR-0039](./ADR-0039-nested-ink-box-rendering.md)
   outcome, then clipped to ancestor natural-AABB clips.** Local/uncomposed bounds are
   illegal keys. Overflow past a parent natural AABB is not hittable — the indexed box
   is the **clipped** hull, so a point outside an ancestor does not return that child.
   Connectors are indexed (AABB of the warped path) so marquee/freeform can cull them;
   exact ≥80% is still **path samples**, not AABB ([BR-C11](../modules/epaper/features/connector-ink/srs-product.md)).
   Tap does not select a connector from its AABB.

3. **Invalidation**
   - **Op commit / accepted `doc_load` / undo apply:** rebuild (STR pack of all current
     entries). 500 nodes is cheap; prefer a full rebuild over a live incremental R* until
     a measured miss files a `CHL-*`.
   - **Live move / resize preview:** **0** index writes. The tree is not mutated until
     release ([SRS-EP-11](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation)
     one op per gesture). Move-reparent runs **at commit**, against the post-move geometry,
     then the rebuild happens with that commit. The index must not run on the ink/paint
     thread.

4. **Z-order / nesting is not AABB-max.** Children-before-ancestors and later-siblings-first
   are **paint-order among candidates**, using `paintRank` / `depth` assigned at rebuild by
   the same later-first DFS as [SRS-EP-77](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent).
   The R-tree returns the AABB-containing set; the winner is the nested walk **restricted
   to that set**. A larger AABB must not beat a later, deeper child.

5. **Index culls; exact tests stay O(k) on the candidate set.**
   - Point: AABB-contains, then exact nested tap (clipped AABB + LOD).
   - Rect / polygon 80%: AABB-overlap with the query (polygon uses its AABB as the
     probe), then the **existing** ≥80% sample-count / AABB-area / 5×5-grid tests.
   - Highest-paint container ≥80% of a rectangle: AABB-overlap cull, then even-odd
     boundary (SmartGroup) or AABB (Frame/Group) on the moving node’s natural area.
   - Draw-into: cull SmartGroups whose AABB overlaps the stroke AABB, then ≥80%
     **polyline length** in boundary ink (nested groups included).
   - Enclose capture: cull top-level candidates overlapping the fitted AABB, then
     existing ≥80% sample / natural-area tests.
   Never replace an 80% sample or length test with AABB-overlap-alone.

6. **Object erase 80% table ([SRS-EP-58](../modules/epaper/features/erase/srs-logic.md#srs-ep-58-object))
   shares the index for AABB cull only.** Its 80% is a **different** metric (ink **arc
   length**, SmartGroup **boundary-polygon area**, connector warped length). Exact tests
   stay in the erase table. Overlay 80% stays on the worker ([ADR-0036](./ADR-0036-toolcanvas-live-overlay.md));
   the index is not queried on the pointer-move path.

7. **Not this index.** Clipboard paste **20% ancestor overlap** ([BR-C13](../modules/epaper/features/clipboard/srs-product.md))
   is O(depth) after the tap point-query — do not range-scan the tree. Endpoint-ink
   connector collection and area-erase clip may **optionally** cull later; they are not
   required callers this bind. Infini’s paint quadtree is **not** reused on device (Decision §8).

8. **Relationship to [ADR-0010](./ADR-0010-tree-of-vectors.md).** Device hit-test index is
   **device-only**, keyed by **nodes** (composed+clipped AABB), rebuilt on **op commit**.
   Infini’s quadtree is keyed by **drawable leaves**, rebuilt / queried for **viewport
   cull**. Do not wait on a shared runtime. Algorithmic fixtures (point / rect / 80%) may
   later be shared; hosts stay split. Nested overlapping SmartGroup AABBs are a bad fit
   for a quadtree (straddling items collapse to the root) — that is why the device
   structure is an R-tree, not Infini’s quadtree copied over.

## Consequences

- Callers go through [SRS-EP-79](../modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries).
  A new linear `for (rootChildren)` geometry walk is a defect.
- Rebuild cost is O(n log n) at commit, not per frame. Accepted as long as
  [SRS-EP-78](../modules/epaper/features/device-document/srs-quality.md#srs-ep-78-log-hit-test)
  rebuild rows hold and ink p95 is untouched.
- **Trade-off point:** exact 80% on k candidates (correctness, k usually local) vs
  approximate AABB-only 80% (faster, **changes product**). Exact wins. If k is huge
  (pathological overlap of the whole page), that is a measured miss → `CHL-*`, not a
  silent threshold change.
- **Sensitivity:** overlap factor (nested boxes along one point). R-tree query is
  O(log n + k); k is overlapping AABBs, not n. Deep nesting is already accepted as
  O(depth) affine in ADR-0039.
- Infini desktop `pickSmartGroupAt` remains a linear walk until a separate Infini
  story. Device is the writer ([ADR-0014](./ADR-0014-document-ownership-inversion.md));
  this ADR does not require a mirror index.

## Alternatives Considered

| Approach | Perf (query) | Ink safety | Correct nested z | Maintain | Why |
|---|---|---|---|---|---|
| **A. One device R-tree, STR rebuild, exact 80% on k (chosen)** | + O(log n + k) | + (commit-only rebuild) | + paint-rank among candidates | + one API | Overlapping MBRs are the nested-box case |
| B. Status-quo linear walk | − O(n) per query | + (nothing extra) | + (today’s walk) | − N copies of the walk | Rejected — the request is logarithmic cost vs n |
| C. Infini paint quadtree reused / ported | 0 / − (overlap degenerates) | 0 (different rebuild) | − leaves ≠ nodes | − two hosts, wrong keys | ADR-0010 index is cull-of-primitives, not hit-of-nodes |
| D. Quadtree / grid on device | − nested overlap sits on internal nodes | + | 0 | 0 | Same degeneration as C for nested SmartGroups |
| E. Interval trees (x and y) | 0 | + | − no paint-order | 0 | Extra join; still not 2D-overlap native |
| F. Z-order / Hilbert skip list of AABBs | 0 (points, not overlapping rects) | + | − | − | Poor for nested overlapping hulls |
| G. Approximate 80% from AABB overlap | ++ | + | 0 | 0 | **Product change** — stop and file `CHL-*` |
| H. Live-gesture incremental index | 0 | − (steals ink / overlay) | 0 | − | Reparent is commit-only; preview must not write the index |

**Sensitivity point:** k (overlapping AABBs at the query). **Trade-off point:** exact 80%
vs approximate. Exact is pinned. Status quo (B) is the rejected alternative the human
asked to replace.
