---
feature: region-sync
parent_req: [REQ-02]
version: 0.2.0
lifecycle: active
---

# SRS — Region sync Epaper (Quality)

## [SRS-EP-03] Map-before-refresh and op fidelity

| Scenario | Metric | Target |
|---|---|---|
| Viewport received → next pen sample world mapping | Must use new viewport | Always |
| Region refresh vs Infini document hash for AABB | Equal content | Always (debug assert) |
| Local ink → `append_ink` emitted | Channels present on wire match captured sample | 100% of reported channels |
| Hot path | Socket I/O must not run on pen sample callback | Always (queue to net thread) |
| Remote op apply | Idempotent on `opId` | Always |
