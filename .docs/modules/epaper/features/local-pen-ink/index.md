---
feature: local-pen-ink
parent_req: [REQ-01]
lifecycle: active
module: epaper
---

# Feature — Local pen-matched ink

On-device Qt Quick app that receives tablet events and paints ink on the RM2
e-paper panel with pen-matched coordinates.

- Logic: [srs-logic.md](./srs-logic.md)
- Implements: `Epaper/` (Qt/C++, not SwiftPM)
- Proven in: [EXP-0001](../../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
