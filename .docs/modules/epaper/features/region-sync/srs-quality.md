---
feature: region-sync
parent_req: [REQ-02]
version: 0.1.0
lifecycle: active
---

# SRS — Region sync Epaper (Quality)

## [SRS-EP-03] Map-before-refresh

| Scenario | Metric | Target |
|---|---|---|
| Viewport received → next pen sample world mapping | Must use new viewport | Always |
| Region refresh vs Infini document hash for AABB | Equal content | Always (debug assert) |
