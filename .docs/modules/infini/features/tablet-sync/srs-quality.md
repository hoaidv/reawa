---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.4.0
lifecycle: active
---

# SRS — Tablet sync Infini (Quality)

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

## [SRS-IN-08] Latency, consistency, and paint parity

| Scenario | Metric | Target |
|---|---|---|
| Stroke sample leaves Epaper → visible on Infini | p95 | ≤ 50 ms |
| Viewport change → Epaper map applied | p95 | ≤ 100 ms to map apply; panel refresh may trail |
| Viewport publish under gesture spam | Outbound rate | ≤ 30 Hz; settle pose always flushed (`settle: true`) |
| Marker visibility | Idle vs gesturing | Hidden when idle; visible while gesturing; hide ~100 ms after settle |
| `doc_snapshot` vs Epaper raster | WorldLayer figures ∩ region | 0 divergent figures after sharp settle |
| Stroke width parity | World × Infini scale vs world × `s_panel` | ≤ 5% relative error @ matched region scale |
| Zoom stroke feel | Halve Infini scale (or grow region world extent) | Apparent stroke thickness halves on Infini **and** Epaper (live + stored) |
| Gut orientation | Tall default / wide cycle | Vertical `gutToLeft` correct; wide not L/R mirrored |

### Dual-ask

REQ-03 Needs design: no — marker is a gesture affordance. Optional connection indicator
may ride infinity-canvas chrome later.
