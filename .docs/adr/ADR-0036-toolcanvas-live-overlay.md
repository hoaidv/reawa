---
id: ADR-0036
title: ToolCanvas live overlay — append-only raster and off-thread hits
status: accepted
date: 2026-08-31
deciders: [architect]
supersedes: null
amends: [ADR-0019, ADR-0033, ADR-0034]
source: STORY-EP-066
---

# ADR-0036 — ToolCanvas live overlay: append-only raster and off-thread hits

## Context

Object erase ([SRS-EP-58](../modules/epaper/features/erase/srs-logic.md#srs-ep-58-object)) draws a
live dotted freeform plus AABB **deletion-rects** on ToolCanvas ([ADR-0019](./ADR-0019-selection-chrome-layers.md)).
Field passes froze or lagged the **polyline** when:

1. Point-in-polygon 80% ran on the UI / pointer thread (Θ(lasso vertices) × boxes).
2. Each pointer event **rebuilt** a `QPainterPath` from every sample and stroked it.
3. `drawLine` per sample with `DotLine` reset the dash — the path looked **solid**.
4. Adding/removing a deletion-rect `update()`’d the whole AABB, which **cleared and restroked**
   every polyline sample that crossed that box.

Brush erase already stamps a persistent ghost `QImage` ([SRS-EP-56](../modules/epaper/features/erase/srs-logic.md#srs-ep-56-brush)). Object erase uses the same class of overlay for the dotted lasso.

Connector UX2 on a page with several ink-boxes plus a grid froze when recognition walked a join
**graph** (DFS / consecutive-tail). That is a recognition cost, not a ToolCanvas cost, but it
showed up in the same field pass.

## Decision

1. **Pointer-move is append-only.** `onMove` stores the sample, stamps **one segment** onto an
   overlay raster, dirties that segment. No 80% test, no full-path rebuild, no document walk.

2. **Dotted = one dash-continuous stroke.** Stamp with `CustomDashLine` and a running
   `dashOffset`. Never `drawLine` each sample with a fresh `DotLine` (dash restarts → solid).

3. **Paint blits the raster.** `paintOverlay` `drawImage`s the clip of the cached polyline, then
   draws deletion-rects. Restoring pixels after a rect update is a blit, not a restroke.

4. **Deletion-rect dirty is the outline, not the AABB interior.** Four strips via
   `damageChromeSegment`. Do not `damageChrome` the union of hit AABBs. Note: `QQuickPaintedItem`
   still **unions** dirty rects in one frame into a bounding box, so the panel may refresh that
   AABB. That is e-ink. CPU restroke of a long lasso on rect add/remove is **forbidden**.

5. **80% is off the UI thread.** Snapshot AABB-culled subjects on a timer; [`LatestJob`](../../epaper/util/latest_job.hpp)
   runs PIP on a low-priority worker (one in-flight, at most one pending; newer request replaces
   pending and cancels in-flight). Overlay skips Ink and Connector. Walk **Frame** children only
   — do not enter SmartGroup or Group. Compute may coarsen the lasso and each SmartGroup
   **boundary** polyline; paint and commit-lasso stay full samples. Commit 80% stays the product
   table (boundary area, not fitted AABB).

6. **Connector UX2 is not a graph search.** Last 3 free inks in paint order (skip boxes in
   z-order). Try **B–A–C**, then two-arm **B–C**. No DFS. See [SRS-EP-17](../modules/epaper/features/connector-ink/srs-logic.md#srs-ep-17-connector-recognition).

## Consequences

- ToolCanvas Image `update(rect)` still **clears** that rect (`fillColor` transparent); the blit
  restores the lasso from `m_polyRaster`. Do not “optimize” by restroking samples in `paintOverlay`.
- E-ink must still refresh pixels of a deletion-rect (and, if dirty rects union, its AABB). A
  remaining hitch when a candidate appears is **panel waveform**, not lasso restroke. Non-coalesced
  ToolCanvas dirty regions would be a new challenge, not a silent fork.
- `LatestJob` is the pattern for other live overlay geometry (do not stack one compute per
  pointer event).
- A change that restrokes the live lasso, runs 80% on the pointer path, DFS-walks connector
  joins, or enters SmartGroup/Group for object-erase overlay **stops** and files
  `.plan/iter-*/challenges/CHL-*.md`.

## Alternatives considered

| Approach | Why not |
|---|---|
| Clip-cull `drawLine` of samples in `paintOverlay` | Looks solid; still O(n) when clip is a large AABB |
| Full-path `QPainterPath` every paint | Fine for short lassos; dies on long concave drags |
| DFS longest join-path for UX2 | Too expensive with 2+ SmartGroups (grid freeze) |
| Damage filled AABB of each 80% candidate | Wipes polyline pixels; huge Mono update |
