---
from: pm
to: architect
iter: iter-003
date: 2026-08-11
subject: ink-box-membership-fixedInk-ux
cc: [sm, designer, qa]
verdict: READY-WITH-CONCERNS
---

# PM → Architect — draw-into membership + `fixedInk` centroid rule

Human added three UX rules on top of the adopted ink-box model. Product docs updated; no
campaign lock change (still Infini logic).

## Adopted rules

| Rule | Where |
|---|---|
| New `Pen` ink ≥80% inside a Smart Group → becomes `content` of that box | [REQ-04](../../../.docs/modules/infini/prd.md#smart-group) · BR-09g · [SRS-IN-15](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-15-draw-into-membership) · [ADR-0011 §7](../../../.docs/adr/ADR-0011-smart-group.md) |
| Nested / multi-qualify → highest paint/z order (tree sibling order; no z-index field) | same |
| Appending ink never reflows/shifts existing content (free layout) | BR-09h · Non-Goal: in-box alignment deferred |
| `fixedInk`: keep sample size; offset so content **centroid UV** in the box is preserved on resize | BR-09i · ADR-0011 §3 clarified |

## Concern for you

Library `smartLocalToWorld` today for `fixedInk` is `local + translate` (no scale, **no**
centroid offset). That freezes content near the top-left as the box grows — which PM rejected.
Closing SRS-IN-11 / transform stories must implement the §3 centroid rule (or an equivalent
content offset), not the current helper as-is.

Membership (SRS-IN-15) depends on tree-backed `append_ink` the same way enclose does — sequence
after the ink-ingestion prerequisite.

## Ask

Confirm SRS-IN-15 + ADR-0011 §3/§7 are implementable as written (especially nested z-order =
sibling paint order). Then SM can add an implement story for membership alongside enclose.
