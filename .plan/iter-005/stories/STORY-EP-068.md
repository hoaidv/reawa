---
id: STORY-EP-068
title: Operations own overlay paint; ToolCanvasContext stays generic
kind: implement
parent_srs: [SRS-EP-04, SRS-EP-12, SRS-EP-56]
parent_req: [REQ-03, REQ-06, REQ-11]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given ToolCanvas paint, When any exclusive tool is armed, Then ToolCanvasContext::paintOverlay only dispatches (locked Operation paintOverlay, then settled ToolChrome if still shared) and contains 0 exclusive-tool branches (no erase_brush hover draw, no sel_freeform waveform ifs)."
  - "Given erase_brush pen-near hover, When the circle is shown, Then BrushEraseOperation (or a helper it owns) paints it; ToolCanvasContext has 0 erase millimetre constants and 0 m_eraseHover* fields."
  - "Given sel_freeform or sel_rect during the gesture (Selecting), When the dotted path or rect is shown, Then it is still LassoOperation::paintOverlay / MarqueeOperation::paintOverlay; ToolChrome::paint still no-ops that phase."
  - "Given a new Operation (area erase, object erase), When it needs live overlay, Then it implements paintOverlay and asks ToolContext for damage / host size / panelToWorld / stroke waveform — it does not add fields or draw calls to ToolCanvasContext."
  - "Given overlay presence (Pen vs Content), When an Operation needs Pen waveform, Then it declares that via a generic Operation or ToolContext port (for example wantsPenWaveform), not a list of exclusive-tool ids in ToolCanvasContext::syncOverlayPresence."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-068 — Operations own overlay paint; ToolCanvasContext stays generic

Stop `ToolCanvasContext` becoming the god class for tool drawing. Live overlay already belongs on `Operation::paintOverlay` (lasso, marquee, brush ghost). Hover and exclusive-tool waveform special cases still sit on the context and will grow with area/object erase.

Do this **before** [STORY-EP-065](./STORY-EP-065.md) / [STORY-EP-066](./STORY-EP-066.md) land dotted freeform / AABB chrome. Human is Quality Assurance Engineer; no design package; no behavior-driven ceremony.

Canonical: [SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-04) routing; [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-12-selection-chrome) overlay; [ADR-0033](../../../.docs/adr/ADR-0033-tool-abstraction.md) Operations do not bind Qt, one lock; catalog [tool-system/catalog.md](../../../.docs/modules/epaper/tool-system/catalog.md) (update that page in the same change).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — (can run after or beside [STORY-EP-067](./STORY-EP-067.md); do not wait on area/object) |

## Split today (do not invert)

| Layer | Owns |
|---|---|
| `Operation::paintOverlay` | In-flight gesture: lasso polyline, marquee rect, brush ghost |
| `ToolChrome::paint` | Settled selection AABB / knobs; live manip subtree; **returns immediately while Selecting** |
| `ToolCanvasContext` | Ports: damage, refresh, panel↔world, host size, dispatch paint, waveform |

Move **out** of `ToolCanvasContext`: `paintEraseHover`, `m_eraseHover*`, `eraseMmToWorld` includes, `exclusiveTool() == erase_brush` / `sel_freeform` branches in `syncOverlayPresence`.

Keep **in** `ToolCanvasContext`: generic `paintOverlay` loop (locked op, then chrome), `damageChrome*`, `requestChromeRefresh`, transforms. Settled chrome may stay on `ToolChrome` (not an Operation) unless a later story splits that too.

## Done when

- Adding area/object overlay does not touch `tool_canvas_context.cpp` except registration
- Catalog `Contexts` row matches: adapter + ports, not per-tool draw
- Brush hover and selection-gesture paint still match current panel behaviour (human confirm)
- **Human verified 2026-08-31** on device
