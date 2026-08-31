---
captured: 2026-08-31
related:
  - STORY-EP-066
  - SRS-EP-17
  - SRS-EP-58
  - ADR-0036
---

# Object erase overlay and connector UX2 — do not repeat

Normative: [ADR-0036](../adr/ADR-0036-toolcanvas-live-overlay.md),
[SRS-EP-58](../modules/epaper/features/erase/srs-logic.md#srs-ep-58-object),
[SRS-EP-17](../modules/epaper/features/connector-ink/srs-logic.md#srs-ep-17-connector-recognition).

If a change would violate those pages, file `.plan/iter-*/challenges/CHL-*.md`. Do not silently fork.

## Object erase — what froze / lagged

| Symptom | Actual cause | Forbidden “fix” |
|---|---|---|
| UI freeze on concave lasso | Live 80% PIP on the pointer thread, Θ(L)×boxes; SmartGroup 8×8 vs full boundary | Run PIP in `onMove` |
| Freeze with 2+ ink-boxes + grid | Connector UX2 DFS + dense polyline join | Bring back graph search / “last 5 consecutive siblings” |
| Polyline looks **solid** | `drawLine` per sample with a new `DotLine` (dash restarts on each short segment) | Clip-cull per-segment `drawLine` for “cheap dotted” |
| Polyline lags as the drag gets long | Rebuild/stroke **all** samples on every ToolCanvas `paint` | `QPainterPath` from `m_pts` every event |
| Random hitch when a deletion-rect appears | `update(AABB)` clears that rect; paint restroked the lasso through the box | `damageChrome` of the union of all hit AABBs |

## Object erase — required shape

- **Move:** append sample, stamp **one** dashed segment onto `m_polyRaster` with running `dashOffset`, dirty that segment.
- **Paint:** blit the raster clip, then draw deletion-rects. Never restroke samples.
- **Deletion-rect dirty:** outline strips (`damageChromeSegment`), not the AABB interior. `QQuickPaintedItem` may still union those strips into a bounding box in one frame — e-ink of that box can hitch; **CPU restroke of the lasso must not**.
- **80%:** timer + `LatestJob` (low-priority worker). One in-flight, one pending, replace pending. Snapshot on UI; PIP off UI.
- **Walk:** Frame children only. Do not enter SmartGroup (test the box, not children) or Group.
- **Compute copy** may downsample lasso + SmartGroup boundary. **Paint and commit lasso** stay full samples. Overlay skips Ink and Connector.

Brush ghost (`BrushEraseOperation::m_ghost`) is the same class of append-only raster — copy that, do not invent a per-event path rebuild.

## Connector UX2

Last **3 free inks** in paint order (skip ink-boxes / connectors / membership in z-order). Roles:

1. **B–A–C** — B and C each snap exactly one end to a **different** box; A snaps to none and joins both (`R_JOIN` 6 u or intersect).
2. Else **B–C** two arms that join (no bridge). Prefer the other arm **newest** in paint order.
3. Else UX1 on the current stroke only.

No DFS. No “last 5 consecutive root siblings, stop at box.” Older free inks outside the last-3 window are ignored.
