---
id: ADR-0012
title: World-space stroke width and viewport paint parity
status: accepted
date: 2026-08-11
deciders: [architect, pm]
supersedes: null
---

# ADR-0012 — World-space stroke width and viewport paint parity

## Context

[REQ-03](../modules/infini/prd.md#tablet-sync) / Epaper
[REQ-02](../modules/epaper/prd.md#region-sync) require the tablet drawing region to show the
**same picture** as Infini for that region ([ADR-0009](./ADR-0009-shared-document-viewport.md)).
Human reports stroke thickness looks wrong vs the tablet-sync region size today, and zoom
must shrink/grow strokes on the tablet the way Infini already does on desktop.

Quality goals at stake: visual parity (fidelity), map/paint correctness under zoom, and
e-ink refresh cost (do not re-derive width in device pixels independently per platform).

Parents: [SRS-IN-07](../modules/infini/features/tablet-sync/srs-logic.md),
[SRS-EP-02](../modules/epaper/features/region-sync/srs-logic.md),
[SRS-IN-04](../modules/infini/features/vector-document/srs-logic.md) ink style.

## Decision

1. **Stroke geometric width is world units.**  
   Ink `style.strokeWidth` (and any pressure→width curve output) is stored and transmitted in
   **document world space**, never as “CSS px” or “panel px”.

2. **Paint width follows viewport / region scale.**  
   - Infini: `lineWidth_css = strokeWidth_world * viewport.scale` (same rule as primitives
     with `strokeScale = 1` world path).  
   - Epaper: when the panel maps onto `drawingRegion` of world width `W_world` and panel
     width `W_px`, `lineWidth_px = strokeWidth_world * (W_px / W_world)` (and likewise for Y
     when aspect is locked — use the uniform scale implied by the AABB→panel fit).

3. **Zoom changes apparent size on both peers.**  
   Increasing Infini `scale` (or shrinking `drawingRegion` world extent for a fixed panel)
   makes the same world stroke **thicker in device pixels**; zooming out makes it thinner.
   Do **not** keep screen-constant stroke width on either peer for synced ink.

4. **Pressure modulates world width before paint.**  
   Optional: `width_world = f(baseWidth_world, pressure)` evaluated once per sample or stroke;
   both peers use the same `f` for live + replay.

## Consequences

- Fixes EXP-era mismatch where brush width was treated as device/CSS constants.
- Epaper rasteriser must scale line width by panel/world ratio; Infini must not paint synced
  ink with a hard-coded screen `strokeWidth` that ignores `viewport.scale`.
- Smart Group `inkScaleMode` still applies to **sample positions** under group TF
  ([ADR-0011](./ADR-0011-smart-group.md)); base stroke width remains world units after flatten.
- QA can assert: same fixture ink + same `drawingRegion` → relative thickness vs region
  edges matches within tolerance ([SRS-IN-08](../modules/infini/features/tablet-sync/srs-quality.md) /
  [SRS-EP-03](../modules/epaper/features/region-sync/srs-quality.md)).

## Alternatives Considered

| Approach | Parity | Zoom feel | Sync simplicity | Notes |
|---|---|---|---|---|
| **World width × viewport/region scale** | + | + (matches Infini) | + | **Chosen** |
| Screen-constant width on both | − vs region | − zoom-out keeps fat strokes | 0 | Rejected — breaks “draw anywhere / zoom” feel |
| Device-native width on Epaper only | − | − | − dual semantics | Rejected — human-reported mismatch |
| Encode width as % of drawingRegion | 0 | + | − fragile | Rejected — extra schema; world units already scale |
