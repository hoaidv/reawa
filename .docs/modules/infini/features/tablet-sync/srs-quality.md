---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.1.0
lifecycle: active
---

# SRS — Tablet sync Infini (Quality)

## [SRS-IN-08] Stroke visibility latency

| Scenario | Metric | Target |
|---|---|---|
| Stroke sample leaves Epaper → visible on Infini | p95 | ≤ 50 ms (EXP S2) |
| Viewport change → Epaper map applied | p95 | ≤ 100 ms to map apply (EXP S3); panel refresh may trail |
