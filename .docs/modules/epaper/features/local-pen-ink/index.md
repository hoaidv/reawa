---
feature: local-pen-ink
parent_req: [REQ-01, REQ-11]
lifecycle: active
module: epaper
---

# Feature — Local pen-matched ink

On-device Qt Quick app that receives tablet events and paints ink on the RM2
e-paper panel with pen-matched coordinates.

- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-01] · [SRS-EP-27] **retired** (Path A; see [erase](../erase/index.md))
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-30] **retired** (see [SRS-EP-59](../erase/srs-logic.md#srs-ep-59-erase-quality))
- Implements: `epaper/` (Qt/C++, not SwiftPM) — see [RENDERING.md](../../../../epaper/RENDERING.md)
- Proven in: [EXP-0001](../../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
