---
id: STORY-EP-002
title: Viewport apply + e-ink coalesced region refresh
kind: implement
parent_srs: [SRS-EP-02]
parent_req: [REQ-02]
status: in-review
priority: P1
iter: iter-002
estimate: 3
owner: dev
depends_on: [STORY-EP-001, STORY-IN-011]
acceptance_criteria:
  - "Given Epaper has last-good viewport map M0, When Infini sends a new viewport (translate, scale, drawingRegion, seq), Then input→world and ink transform update before the next pen sample."
  - "Given a burst of viewport messages during Infini pan/zoom, When Epaper receives them, Then map applies each latest immediately while region refresh is coalesced/throttled to an e-ink-safe rate (architect budget in SRS-EP-03)."
  - "Given document ops and/or viewport change require content under the region, When a coalesced refresh runs, Then the panel paints current document ∩ current drawingRegion (same-picture rule)."
  - "Given world ink with width W and viewport scale S, When Epaper rasterises strokes in the drawing region, Then on-panel stroke thickness matches Infini’s view of that region (shared world-unit + scale rule); zoom out shrinks apparent stroke size, zoom in enlarges it."
  - "Given ADR-0009 session is active, When viewport path is wired, Then legacy StrokeSync host path does not own the drawing-region map (migrate or disable per SRS-EP-02)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-002 — Viewport apply + e-ink coalesced region refresh

Parent: [SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md) ·
[SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md) ·
[ADR-0012](../../../.docs/adr/ADR-0012-world-stroke-viewport-parity.md).

## Architect notes

- Fix `panelToWorld` → normalize panel → `drawingRegion`
- Refresh ≥250 ms; settle flush ≤100 ms
- World stroke width × `s_panel`

## Done when

- BDD / host tests green; RegionSession coalesce + map fixed.
