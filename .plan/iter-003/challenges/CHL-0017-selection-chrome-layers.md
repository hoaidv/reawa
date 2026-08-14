---
id: CHL-0017
title: Selection chrome collapsed onto document canvas — lasso lag
author: architect
target: [REQ-06, SRS-EP-12, SRS-EP-14, ADR-0016, STORY-EP-018]
severity: high
status: resolved
resolution: adopted
resolved_by: architect
resolved: 2026-08-14
opened: 2026-08-14
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human — sel_freeform boundary lag on RM2
---

# CHL-0017 — Selection chrome collapsed onto document canvas (lasso lag)

## Context

Human (2026-08-14): **`tool.sel_freeform` is laggy while drawing the boundary.** Architect probe:

- Live lasso / marquee / settled dotted AABB / handles / ink-scale chip are painted in
  `TabletCanvasItem::paint()` after `drawImage(m_image)` — **same Qt item as the document**.
- Each lasso move calls `update()` with **no dirty rect** (full item). Move/resize already uses a
  regional dirty + 50 ms throttle; ink uses bbox + Pen.
- Waveform on the host is **`EPScreenModeItem(Mode::Pen)`**, not **Mono**. `swapPen` is not used
  on the lasso path. `cta.enclose` and ToolChip are already separate QML items (`z` 21 / 20).

This contradicts [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) composition
(SelectionOverlay **above** InkSurface) and [SRS-EP-14](../../../.docs/modules/epaper/features/ink-box/srs-quality.md)
refresh discipline (`sel.lasso` = Partial; 0 full-panel invalidations). Closed control inventory
and SmartGroup logic are **not** the defect.

## Proposal

Adopt a **refresh-class** split (human + architect 2026-08-14):

1. **CanvasLayer** — document render (`m_image`, live ink). Waveform **Pen**.
2. **ToolCanvasLayer** — canvas-backed tool strokes: marquee, lasso, settled dotted AABB. Transparent
   sibling `QQuickPaintedItem`. Waveform **Mono**. Tight `update(bbox)`.
3. **ToolLayer** — QML widgets: anchors/handles, Enclose, ink-scale chip, ToolChip, indicators.
   ToolChip is **screen-space**; selection widgets are **content-space**.

Do **not** invade enclose / membership / `set_smart_transform`. One input owner; ToolCanvasLayer
paints; CanvasLayer keeps gestures in the first slice.

## Resolution

**Adopted** 2026-08-14 (architect; human-directed). Recorded as
[ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md).

Locks:

- Product inventory unchanged (no new tools, no context-toolbar revival as a fifth chip).
- Pen / Mono / UI waveforms per layer; no window-wide Pen blanket for chrome.
- Lasso stays `QPainter` polyline — not QML Shape.
- Visual design packages UI-EP-02 / UI-EP-03 remain the hi-fi; no new design story required.

## Product doc updates

- [ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md)
- `srs-ui.md` composition (SRS-EP-12)
- `srs-quality.md` lasso/marquee refresh rows (SRS-EP-14)
- `epaper/architecture.md` paint-stack view + decision link

## Interrupt / expedite

Does **not** freeze TRACK-003. Does **not** preempt W11b EP-019 QA — same `tabletcanvasitem.cpp`
conflict. Slice as the next implement story after EP-019 (or serial behind it).
