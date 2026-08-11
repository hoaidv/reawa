---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.3.0
lifecycle: active
---

# SRS — Tablet sync Infini (Quality)

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

## [SRS-IN-08] Latency, consistency, and paint parity

| Scenario | Metric | Target |
|---|---|---|
| Stroke sample leaves Epaper → visible on Infini | p95 | ≤ 50 ms (EXP S2) |
| Viewport change → Epaper map applied | p95 | ≤ 100 ms to map apply (EXP S3); panel refresh may trail |
| Viewport publish under gesture spam | Outbound rate | ≤ 30 Hz; settle pose always flushed |
| Marker visibility | Idle vs gesturing | Hidden when `ui.gesturing=false`; visible when true |
| `append_ink` apply order | Ops for one stroke appear in sample order | Always |
| Idempotent replay | Same `opId` twice → one node | Always |
| Document ∩ viewport paint | Infini tree vs Epaper refresh for same AABB | 0 divergent strokes (debug hash) |
| Stroke width parity vs region | Relative thickness (stroke CSS width / tablet-frame CSS width) vs (stroke panel px / panel width) for same world ink + region | ≤ 5% relative error @ matched scale |
| Zoom stroke feel | Halve Infini `scale` (or double region world extent for fixed panel) | Apparent stroke thickness halves on Infini **and** on Epaper after map+paint |

### Dual-ask

Sync has **Needs design: no** on REQ-03 — marker is a gesture affordance, not DocChrome.
Optional connection indicator may ride infinity-canvas chrome later without a separate
design story.
