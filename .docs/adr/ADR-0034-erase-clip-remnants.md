---
id: ADR-0034
title: Geometric ink clip and remnant Ink nodes
status: accepted
date: 2026-08-29
deciders: [architect, pm]
supersedes: null
amends: [ADR-0010, ADR-0012, ADR-0025, ADR-0032, ADR-0033]
source: CHL-0028
---

# ADR-0034 — Geometric ink clip and remnant Ink nodes

## Context

[REQ-11](../modules/epaper/prd.md#erase) / [prd-erase.md](../modules/epaper/prd-erase.md) brush and area must punch holes in handwriting. [ADR-0010](./ADR-0010-tree-of-vectors.md) Ink is **one continuous polyline**. Deleting samples in place draws a **chord** across the gap. Path A’s “0 new Ink nodes” / `set_ink_samples` only is therefore wrong for a split stroke.

Quality: 0 chords; enclose/membership 80% stay honest per node; one undo per gesture ([ADR-0032](./ADR-0032-inverse-op-undo.md)); p95 ≤50 ms after pointer-up.

## Decision

1. **Clip is geometry.** Brush: capsule (radius **4 mm** world) along the gesture. Area: even-odd interior of the auto-closed freeform. Intersect each Ink polyline with that region; do **not** approximate by dropping sample points only.

2. **Remnants are nodes.** After clip, each connected leftover with arc length ≥ **1 mm** is its own Ink polyline. Original id keeps the **longest**; extras are `append_ink` siblings (same parent, style, adjacent paint order). Zero remnants → `remove_node`. One gesture → one `compound` when more than one op.

3. **No multi-contour Ink.** One node remains one polyline. Broken SmartGroup **boundary ink** is allowed; the invisible **boundary polyline** is never clipped ([prd-erase.md §11](../modules/epaper/prd-erase.md)).

4. **Live vs commit.** ToolCanvas chrome during the gesture; document mutate **only** at pointer-up.

5. **World millimetres for erase constants.** Erase diameters, remnant floor, and hover stroke in the PRD are **millimetres in document world**. Paint still uses [ADR-0012](./ADR-0012-world-stroke-viewport-parity.md) (world × panel/world scale). A tagged `Dimension` type is **not** introduced in the core tree this wave.

## Consequences

- `set_ink_samples` alone is insufficient when a stroke splits; Infini must apply `append_ink` + `compound`.
- Enclose / draw-into 80% apply per remnant (honest).
- Existing Path A tests that assert 0 new nodes are retired with [SRS-EP-27](../modules/epaper/features/local-pen-ink/srs-logic.md).

## Alternatives Considered

| Approach | Chords | Enclose honesty | Undo | Notes |
|---|---|---|---|---|
| Sample-delete, one array | − | − | + `set_ink_samples` | Rejected |
| One node, many contours | + | − unless every consumer walks contours | + | Rejected — domain rewrite |
| **Split remnants (this ADR)** | + | + | `compound` | **Chosen** |
