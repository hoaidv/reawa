---
id: ADR-0019
title: Selection chrome: CanvasLayer / ToolCanvasLayer / ToolLayer
status: accepted
date: 2026-08-14
deciders: [architect]
supersedes: null
amends: [ADR-0016]
source: CHL-0017
---

# ADR-0019 — Selection chrome: CanvasLayer / ToolCanvasLayer / ToolLayer

## Context

[SRS-EP-12](../modules/epaper/features/ink-box/srs-ui.md) names **InkSurface**, **SelectionOverlay**,
and **ToolChip**. Device code collapsed overlay chrome into `TabletCanvasItem::paint()`: blit
`m_image`, then draw lasso / marquee / dotted AABB / handles / ink-scale chip. Live
`tool.sel_freeform` calls `update()` with no dirty rect on every sample. That violates refresh
discipline ([SRS-EP-14](../modules/epaper/features/ink-box/srs-quality.md): 0 full-panel
invalidations; `sel.lasso` = Partial) and makes the lasso feel laggy.

`cta.enclose` and ToolChip are already sibling QML items. `EPScreenModeItem(Mode::Pen)` covers the
whole canvas; **Mono** (fast 1-bit) is unused. Quality goal 1 (ink ≤30 ms) must stay off this
path. SmartGroup membership / enclose / transforms must not change ([SRS-EP-10](../modules/epaper/features/ink-box/srs-logic.md) /
[SRS-EP-11](../modules/epaper/features/ink-box/srs-logic.md)).

Closed inventory and visual design ([UI-EP-02](../../.plan/iter-003/design/device-selection-chrome/),
[UI-EP-03](../../.plan/iter-003/design/selection-enclose-chrome/)) stay the same. This decision is
**composition + waveform + damage**, not new controls.

## Decision

Split SelectionOverlay by **refresh class** into three Qt surfaces:

| Layer | Product region | Renders | Qt | Waveform | Coordinate space |
|---|---|---|---|---|---|
| **CanvasLayer** | InkSurface | Document `m_image` + live ink during move/resize | Existing `TabletCanvas` | **Pen** | Content |
| **ToolCanvasLayer** | SelectionOverlay stroke chrome | `ovl.marquee`, `ovl.lasso`, settled dotted `ovl.nodes_bounds` / `ovl.selection_bounds` | Sibling transparent `QQuickPaintedItem` | **Mono** | Content |
| **ToolLayer** | SelectionOverlay widgets + ToolChip | Handles/anchors, `cta.enclose`, `tgl.ink_scale_mode`, indicators, ToolChip | QML `Item`s | **UI** (scene default) | **Content** for selection widgets; **screen** for ToolChip |

Locks:

1. **Never blit the document** on ToolCanvasLayer. Never bake chrome into `m_image`.
2. **Damage:** ToolCanvasLayer `update(segmentAABB)` (lasso) or old∪new AABB (marquee/settled).
   CanvasLayer `update()` with no rect is forbidden for chrome. ToolLayer dirties only the widget.
3. **Input:** one owner (today `TabletCanvasItem`). ToolLayer is pickable only on control rects;
   otherwise events fall through. ToolCanvasLayer does not steal pen while `tool.pen` / `ink_box`
   is armed. Preferred first slice: CanvasLayer keeps gesture tracking; ToolCanvasLayer **paints only**.
4. **Do not** implement lasso as QML `Shape` / `Repeater`. Polyline stays `QPainter` on ToolCanvasLayer.
5. Handles and the mode chip **move off** the painted canvas onto ToolLayer (hit-test + cheap damage).
   Enclose/ToolChip stay QML.
6. **No SmartGroup / surround-create logic changes.** Chrome subscribes to `selectedIds` + panel AABB.

Ship order: ToolCanvasLayer for live lasso/marquee/AABB first (lag fix); migrate handles + mode chip
to ToolLayer in the same story if cheap, else a follow-up without a second ADR.

## Consequences

- Ink path stays on CanvasLayer + Pen. Chrome no longer re-rasterizes the document each lasso sample.
- Three `EPScreenModeItem` attachments (or equivalent region tags) instead of one Pen blanket.
- Extra QQuickItem vs one painted item — accepted for refresh isolation.
- Trade-off point: **ink latency vs chrome freshness**. Chrome loses Pen; gains Mono + tight damage.
- Follow-up: implement story under SRS-EP-12; no new design package (inventory unchanged).

## Alternatives Considered

| Approach | Ink latency | Refresh discipline | Maintainability | Cost | Why |
|---|---|---|---|---|---|
| Status quo (chrome in `TabletCanvas::paint` + full `update()`) | − | − | + | 0 | Rejected — causes the lag |
| Dirty-rect only, still one painted item | 0 | + | + | low | Rejected as the long-term model: chrome still composites over a full blit when the SG node is large; waveforms stay Pen |
| **Three refresh-class layers (this ADR)** | + | + | 0 | med | Winner — matches SRS regions; isolates damage/waveform |
| QML-only overlay (lasso as Shape) | − | 0 | 0 | med | Rejected — hundreds of samples vs ~250 ms panel floor |
| Scene-graph `QSGNode` overlay | + | + | − | high | Deferred — split items first |

Sensitivity: **damage rect size** drives SWTCON time. Trade-off: extra items vs one texture.
