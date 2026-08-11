---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.2.0
lifecycle: active
---

# SRS — Tablet sync Infini (Quality)

## [SRS-IN-08] Latency and consistency

| Scenario | Metric | Target |
|---|---|---|
| Stroke sample leaves Epaper → visible on Infini | p95 | ≤ 50 ms (EXP S2) |
| Viewport change → Epaper map applied | p95 | ≤ 100 ms to map apply (EXP S3); panel refresh may trail |
| `append_ink` apply order | Ops for one stroke appear in sample order | Always |
| Idempotent replay | Same `opId` twice → one node | Always |
| Document ∩ viewport paint | Infini tree vs Epaper refresh for same AABB | 0 divergent strokes (debug hash) |

### Dual-ask

Sync has **Needs design: no** on REQ-03 — no DocChrome required. Optional connection
indicator may ride infinity-canvas chrome later without a separate design story.
