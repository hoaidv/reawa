---
feature: region-sync
parent_req: [REQ-02]
version: 0.3.0
lifecycle: active
---

# SRS — Region sync Epaper (Quality)

Parent REQ: [REQ-02](../../prd.md#region-sync).

## [SRS-EP-03] Map-before-refresh, coalesce, and stroke fidelity

| Scenario | Metric | Target |
|---|---|---|
| Viewport received → next pen sample world mapping | Must use new viewport | Always |
| Map apply latency after viewport on wire | p95 | ≤ 100 ms (align SRS-IN-08) |
| Region refresh vs Infini document hash for AABB | Equal content | Always (debug assert) |
| Refresh coalesce under pan/zoom spam | Min interval between region paints | ≥ **250 ms** (≤ ~4 Hz); always paint latest pending pair |
| Gesture / viewport settle | If refresh pending at settle | Flush one refresh ≤ **100 ms** after last viewport seq applied |
| Local ink → `append_ink` emitted | Channels present on wire match captured sample | 100% of reported channels |
| Stroke width | World units on wire; panel px = world × `s_panel` | Always ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)) |
| Zoom parity | Double Infini scale (halve region world width for fixed panel) | Panel stroke px ≈ 2× prior for same world width (±5% relative) |
| Hot path | Socket I/O must not run on pen sample callback | Always (queue to net thread) |
| Remote op apply | Idempotent on `opId` | Always |

### Notes

- **Map** is never coalesced; **paint** is. Ghosting between paints is accepted.
- 250 ms floor is the W5 default for full-region refresh; product may later tune partial
  damage without violating map-before-paint coherency.
