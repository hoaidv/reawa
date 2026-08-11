---
feature: region-sync
parent_req: [REQ-02]
version: 0.4.0
lifecycle: active
---

# SRS — Region sync Epaper (Quality)

Parent REQ: [REQ-02](../../prd.md#region-sync).

## [SRS-EP-03] Map-before-refresh, coalesce, and stroke fidelity

| Scenario | Metric | Target |
|---|---|---|
| Viewport received → next pen sample world mapping | Must use new viewport + gut UV | Always |
| Map apply latency after viewport on wire | p95 | ≤ 100 ms (align SRS-IN-08) |
| Region raster vs Infini `doc_snapshot` for AABB | Equal figures after settle | Always |
| Soft refresh under pan/zoom spam | Min interval between soft paints | ≥ **250 ms**; latest pending wins |
| Settle / `doc_snapshot` | Sharp paint | Immediate; AA on; no soft fade left behind |
| Local ink → wire | `stroke_*` with world brush + panel x/y | Always |
| Stroke width | Live + vector: `world × s_panel` | Always ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)) |
| Zoom parity | Grow region world width (zoom out) | Panel stroke px shrinks for same world width (±5%) |
| Hot path | Socket I/O must not block pen sample callback | Always |
| Bitmap `region_refresh` | Ignored | Always |

### Notes

- **Map** is never coalesced; **paint** is. Ghosting between paints is accepted.
- Library `RegionSession` coalesce semantics remain the target for a future Qt wiring story.
