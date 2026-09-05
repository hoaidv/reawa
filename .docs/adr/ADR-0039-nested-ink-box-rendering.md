---
id: ADR-0039
title: Nested ink-box RenderingContext and own-transform
status: accepted
date: 2026-09-05
deciders: [architect, pm]
supersedes: null
amends: [ADR-0011]
source: TRACK-005 / [CHL-0032] / [REQ-05] [REQ-06] / [SRS-EP-75] [SRS-EP-76] [SRS-EP-77]
---

# ADR-0039 — Nested ink-box RenderingContext and own-transform

## Context

[CHL-0032](../../.plan/iter-005/challenges/CHL-0032-nested-ink-box.md) schedules nested Smart Groups
this campaign. [ADR-0011](./ADR-0011-smart-group.md) v0 said **Ink children only** and world draw =
`groupTransform ∘ localSample` for a **single** group. Paste already inserts a SmartGroup under a
SmartGroup (`insertAt`); paint uses only the **immediate** parent (`walkFlat(…, &node)`), and
`collectPickable` never walks into a SmartGroup. Result: a nested box can paint (world coords /
camera) and still be unhittable.

Quality goals: **gesture truth** (nested tap/move must mutate the child’s own-transform only) and
**document fidelity** (composed paint must match the tree). Ink latency must not regress.

## Decision

1. **Children.** A SmartGroup may contain `Ink` (`role: boundary | content`) **and** nested
   `SmartGroup` children. Empty = only boundary-role Ink (no content-role children, no nested
   groups). Empty children **flatten** on capture / paste-into-box ([SRS-EP-75](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-75-nested-membership)).

2. **Own-transform stays per node.** Each SmartGroup keeps `{ translate, rotation, scaleX, scaleY }`
   (ADR-0011). Manipulating a selected descendant mutates **that node only**. Do **not** bake
   ancestor transforms into child samples on every drag.

3. **RenderingContext.** Paint (and world hit of nested nodes) uses an affine 3×3:

   ```
   Affine { a, b, c, d, e, f }  // [ a c e ; b d f ; 0 0 1 ]
   apply(x,y) = (a x + c y + e, b x + d y + f)
   compose(P, Q) = P ∘ Q        // Q first, then P
   identity = (1,0, 0,1, 0,0)
   ```

   World starts `RenderingContext { transform = identity }`. For a SmartGroup:

   | Step | Rule |
   |---|---|
   | `own` | Affine of `{S, R, T}` as today: scale, then rotate, then translate (bounds origin 0,0) |
   | `outcome` | `ctx.transform ∘ own` |
   | Boundary ink | `outcome` applied to local samples |
   | Content-outcome | `withBounds`: `outcome`. `fixedInk`: `ctx.transform ∘ T ∘ R` (**no** S) — samples keep size; nested children inherit this as their `ctx` |
   | Recurse | `RenderingContext { transform = content-outcome }` for children |

4. **Hit-test.** Tap walks the tree **depth-first, later siblings first**, children before
   ancestors (nested box wins) **only inside** each ancestor’s natural world AABB. Overflow is
   not hittable. Marquee / freeform still use **top-level** pickables only
   ([SRS-EP-77](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)).

5. **Natural area (reparent).** Moving node’s **natural area** = area of its local `bounds`
   rectangle (SmartGroup) or sample AABB (Ink), measured after its **full world outcome**
   (axis-aligned hull of the four transformed corners). Candidate parent = SmartGroup / Frame /
   Group whose **boundary-ink even-odd interior** (SmartGroup) or bounds (others) contains ≥80% of
   that area; exclude self and descendants; **highest paint order** wins; else document root.
   Computed **at move commit**, not during the drag.

6. **Content clip (2026-09-05).** Content of a SmartGroup (content-role ink, nested groups, descendants)
   is clipped to that group’s **natural world AABB** (same hull as (5), not even-odd boundary ink).
   Overflow is neither painted nor tap-hittable. Dirty / hierarchy cull stay on that AABB — do not
   walk descendant handwriting to union a visual AABB. Nested clips **intersect**. Boundary-role ink
   of the group itself is not clipped to *its own* AABB.

## Consequences

- `smartLocalToWorld(sg, …)` becomes `apply(outcome, local)` with an explicit `ctx`; a one-parent
  overload remains as `ctx = identity` for top-level groups.
- `CreateSmartGroupEdit` capture uses `detachAny` (not only `detachInk`) so nested groups move.
- `insertUnder` still rejects SmartGroup (Frame/Group only). Nested insert stays `insertAt` (paste)
  or children listed on `create_smart_group`.
- Infini mirror must compose the same affine **and clip content to the same natural AABB** or nested
  files will drift — shared fixture required.
- Rotation field remains reserved in the product UI; the affine already carries it for flatten-bake.

**Trade-off point:** compose at paint (correct nested move) vs bake world samples on every parent
drag (cheaper paint, destroys own-transform). Compose wins gesture truth.

## Alternatives Considered

| Approach | Perf | Truth | Maintain | Why |
|---|---|---|---|---|
| **A. RenderingContext compose (chosen)** | 0 (one extra 3×3 per node) | + | + | Matches Rule 2; own-transform stays local |
| B. Status quo (immediate parent only) | + | − | − | Nested paste unhittable; parent move drops children visually |
| C. Bake world samples on every ancestor drag | − (rewrite ink) | 0 | − | Destroys nested own-transform; undo-hostile |
| D. Promote nested groups to root + offset | + | − | − | Tree lies; paste-into-box becomes a fake parent |

Sensitivity: nested depth (expected small, handwriting pages). Accept O(depth) affine compose.
