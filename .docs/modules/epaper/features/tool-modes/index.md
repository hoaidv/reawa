---
feature: tool-modes
parent_req: [REQ-03, REQ-10, REQ-11, REQ-17, REQ-18]
lifecycle: active
module: epaper
---

# Feature — On-device tool modes (Epaper side)

Three-tool toolbar (`Selection · Pen · Ink-box`) on the panel, driven by finger touch so the pen
stays free for content. Since 2026-08-13 the tools **act on the device's own document** rather than
emitting intent to a peer ([CHL-0008](../../../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md)).

- Product: [PRD REQ-03](../../prd.md#tool-modes)
- Viewport-follow is [SRS-EP-49](../region-sync/srs-logic.md#srs-ep-49-viewport-follow) / [SRS-EP-50](../region-sync/srs-ui.md) — **not** a ToolChip exclusive, recognizer, or hand-tool tile.
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-04] · [SRS-EP-23] finger tool switch ·
  [SRS-EP-41] barrel dispatch · [SRS-EP-53] on-device map author · [SRS-EP-44] manual create routing
- UI: [srs-ui.md](./srs-ui.md) — [SRS-EP-05] floating ToolChip · [SRS-EP-29] erase chrome ·
  [SRS-EP-42] chip temp tool (**not** the editor) · [SRS-EP-52] on-device map editor (`needs_design: yes`) ·
  [SRS-EP-47] manual create
- Scene graph: [srs-ui-multi-scene.md](./srs-ui-multi-scene.md) — `scene.pen_map_editor` / `_click` / `_hold`
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-06] · [SRS-EP-43] barrel + editor · [SRS-EP-48] manual create
- Runtime (to build): `epaper/Main.qml`, `epaper/tabletcanvasitem.cpp`, `epaper/tabletappfilter.cpp`
- What the tools act on: [device-document](../device-document/index.md) — [SRS-EP-07] ·
  [ink-box](../ink-box/index.md) — [SRS-EP-10] / [SRS-EP-11]
- Local ink it must not slow: [local-pen-ink](../local-pen-ink/index.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1 and §6 (the rest superseded) ·
  [ADR-0025](../../../../adr/ADR-0025-barrel-vs-eraser-nib.md) ·
  [ADR-0030](../../../../adr/ADR-0030-tablet-authors-pen-button-map.md) (supersedes [ADR-0028](../../../../adr/ADR-0028-pen-button-map-settings-channel.md))
