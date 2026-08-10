---
feature: vector-document
parent_req: [REQ-02]
version: 0.1.0
lifecycle: active
---

# SRS — Vector document (Quality)

## [SRS-IN-06] Round-trip fidelity

| Scenario | Metric | Target |
|---|---|---|
| Save SVG → reopen | Vertex / control-point error @ 100% zoom | ≤ 1 CSS px |
| Transmit encode → decode | Op equality on fixture set | 100% |
