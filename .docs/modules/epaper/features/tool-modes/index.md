---
feature: tool-modes
parent_req: [REQ-03, REQ-10, REQ-11, REQ-17, REQ-18, REQ-20]
lifecycle: active
module: epaper
---

# Feature — On-device tool modes (Epaper side)

Three-tool toolbar (`Selection · Pen · Ink-box`) on the panel, driven by finger touch so the pen
stays free for content. Since 2026-08-13 the tools **act on the device's own document** rather than
emitting intent to a peer ([CHL-0008](../../../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md)).

- Product: [PRD REQ-03](../../prd.md#tool-modes) · Device Settings [REQ-20](../../prd.md#device-settings) · Pen buttons [REQ-18](../../prd.md#pen-buttons)
- Viewport-follow is [SRS-EP-49](../region-sync/srs-logic.md#srs-ep-49-viewport-follow) / [SRS-EP-50](../region-sync/srs-ui.md) — **not** a ToolChip exclusive, recognizer, or hand-tool tile.
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-04] · [SRS-EP-23] finger tool switch ·
  [SRS-EP-41] barrel dispatch · [SRS-EP-53] author + persist on device · [SRS-EP-44] manual create routing
- UI: [srs-ui.md](./srs-ui.md) — [SRS-EP-05] floating ToolChip · [SRS-EP-29] erase chrome ·
  [SRS-EP-42] chip temp tool (**not** the Settings page) · [SRS-EP-52] Device Settings · Pen buttons (`needs_design: yes`) ·
  [SRS-EP-47] manual create
- Scene graph: [srs-ui-multi-scene.md](./srs-ui-multi-scene.md) — Keep = `scene.pen_map_editor` only
  ([CHL-0025](../../../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md) adopted:
  one Settings page; drop `present-sheet` / `scene.pen_map_click` / `scene.pen_map_hold`).
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-06] · [SRS-EP-43] barrel + Settings + device persist · [SRS-EP-48] manual create
- Runtime (to build): `epaper/drawing/Main.qml`, `epaper/drawing/ToolCanvas.qml`,
  `epaper/drawing/toolcanvasitem.cpp`, `epaper/drawing/tools/` (Modes / Operations / InputHub),
  `epaper/drawing/tabletcanvasitem.cpp`, `epaper/input/qtinputfilter.cpp`
- Architect view (implementation): [tool-system](../../tool-system/index.md) — [ADR-0033](../../../../adr/ADR-0033-tool-abstraction.md)
- What the tools act on: [device-document](../device-document/index.md) — [SRS-EP-07] ·
  [ink-box](../ink-box/index.md) — [SRS-EP-10] / [SRS-EP-11]
- Local ink it must not slow: [local-pen-ink](../local-pen-ink/index.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1 and §6 (the rest superseded) ·
  [ADR-0025](../../../../adr/ADR-0025-barrel-vs-eraser-nib.md) ·
  [ADR-0031](../../../../adr/ADR-0031-device-settings-persist-on-epaper.md) (supersedes [ADR-0030](../../../../adr/ADR-0030-tablet-authors-pen-button-map.md); persist on device)
