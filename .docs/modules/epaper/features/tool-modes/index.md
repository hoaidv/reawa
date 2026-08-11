---
feature: tool-modes
parent_req: [REQ-03]
lifecycle: active
module: epaper
---

# Feature — On-device tool modes (Epaper side)

Three-tool toolbar (`Selection · Pen · Ink-box`) on the panel, driven by finger touch so the pen
stays free for content. The device carries **intent**, never a document tree or geometry
recognition.

- Product: [PRD REQ-03](../../prd.md#tool-modes)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-04]
- UI: [srs-ui.md](./srs-ui.md) — [SRS-EP-05] · **needs design**
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-06]
- Runtime (to build): `epaper/Main.qml`, `epaper/tabletcanvasitem.cpp`, `epaper/tabletappfilter.cpp`
- Peer: [infini SRS-IN-13 tool intent transport](../../../infini/features/tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport)
- Consumer of intent: [infini SRS-IN-10 / SRS-IN-11](../../../infini/features/vector-document/srs-logic.md)
- Local ink it must not slow: [local-pen-ink](../local-pen-ink/index.md)
- Decision: [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) (amends [ADR-0011](../../../../adr/ADR-0011-smart-group.md))
