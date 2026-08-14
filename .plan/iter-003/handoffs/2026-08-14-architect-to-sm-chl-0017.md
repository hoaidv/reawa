---
from: architect
to: sm
date: 2026-08-14
iter: iter-003
verdict: READY-WITH-CONCERNS
---

# Handoff — architect → sm (CHL-0017 / ADR-0019)

Human: `sel_freeform` lasso lag. Architect: chrome was painted on the document `QQuickPaintedItem`
with full `update()` and Pen waveform.

## Verdict

**READY-WITH-CONCERNS**

| Finding | Class | Evidence |
|---|---|---|
| Refresh-class split matches SRS-EP-12 regions | Strength | ADR-0019; srs-ui composition updated |
| Lasso/marquee full-panel invalidations now a numbered bar | Strength | SRS-EP-14 rows |
| SmartGroup logic out of scope | Strength | CHL-0017 lock |
| Three `EPScreenModeItem` (Pen/Mono/UI) unproven on 3.28 | Concern | Accept: first slice can Mono-tag ToolCanvasLayer only; fall back to Pen region + tight bbox if Mono attach fails — file a follow-up CHL, do not silently revert to full `update()` |
| Same files as EP-019 (`tabletcanvasitem.cpp`) | Concern | SM: do not parallel with W11b |

## Artifacts

- [CHL-0017](../challenges/CHL-0017-selection-chrome-layers.md) adopted
- [ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md) accepted
- SRS-EP-12 composition, SRS-EP-14 quality, `epaper/architecture.md`

## Slice

One **implement** story (no new design package — UI-EP-02/03 inventory unchanged): ToolCanvasLayer
for lasso/marquee/AABB + migrate handles/mode chip to ToolLayer if cheap. After EP-019.

Next: `/sm`.
