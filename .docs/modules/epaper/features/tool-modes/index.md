---
feature: tool-modes
parent_req: [REQ-03]
lifecycle: active
module: epaper
---

# Feature — On-device tool modes (Epaper side)

Three-tool toolbar (`Selection · Pen · Ink-box`) on the panel, driven by finger touch so the pen
stays free for content. Since 2026-08-13 the tools **act on the device's own document** rather than
emitting intent to a peer ([CHL-0008](../../../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md)).

- Product: [PRD REQ-03](../../prd.md#tool-modes)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-04]
- UI: [srs-ui.md](./srs-ui.md) — [SRS-EP-05] floating ToolChip · **needs design**
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-06]
- Runtime (to build): `epaper/Main.qml`, `epaper/tabletcanvasitem.cpp`, `epaper/tabletappfilter.cpp`
- What the tools act on: [device-document](../device-document/index.md) — [SRS-EP-07] ·
  [ink-box](../ink-box/index.md) — [SRS-EP-10] / [SRS-EP-11]
- Local ink it must not slow: [local-pen-ink](../local-pen-ink/index.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1 and §6 (the rest superseded)
