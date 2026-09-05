---
id: CHL-0032
author: pm
target: [REQ-05, REQ-06, SRS-EP-10, SRS-EP-11]
severity: high
status: adopted
opened: 2026-09-05
iter: iter-005
expedite: false
interrupts_track: ""
resolution: adopted
resolved_by: pm
resolved: 2026-09-05
raised_by: human
source: device verify after clipboard paste + empty-letter enclose
---

# CHL-0032 — Nested ink-boxes (this campaign)

## Context

Two shipped defects, one parked capability:

1. **Paste an ink-box into an ink-box** (clipboard [STORY-EP-044](../stories/STORY-EP-044.md)):
   the child is visible and paints correctly when the camera moves, but it is **not selectable,
   movable, or resizable**. Nesting exists in the tree; hit-test / manipulation do not.
2. **A handwritten letter** (O, D, …) is sometimes recognized as an **empty ink-box** (boundary
   only). Enclose of surrounding writing **skips** that box ([CHL-0011](../../iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md)
   flat capture), so the letter is left behind when the cluster is moved or resized.

[CHL-0011](../../iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md) already **adopted nested
enclose as product intent** (2026-08-13) and parked it off TRACK-003 / this lock’s Non-Goals.
Human 2026-09-05 **schedules it now**, with five rules that also fix the two defects.

Applies to **both** creation paths: ink-box recognizer (Creation A) **and** Enclose on the
selection context toolbar (Creation B).

## Proposal

Adopt nested ink-boxes **this campaign**. Five product rules (verbatim lock-in):

**Rule 1 — Flatten empty children.** If a child ink-box is empty (only boundary-ink), convert the
whole ink-box into free-ink: the parent receives that boundary-ink as a **direct** child. No
redundant empty layer. If the child was resized/rotated, bake that transform into the ink first,
then add the outcome as a direct child of the parent.

**Rule 2 — Nested render.** Parent transform still applies to **all** descendants. Each ink-box
has its own-transform. Introduce `RenderingContext { transform }`. World starts at identity.
An ink-box at any level: `outcome-transform = RenderingContext.transform * own-transform`; that
applies to boundary-ink; content may or may not take scale (`inkScaleMode`) →
`content-outcome-transform` is passed down as the children’s `RenderingContext`.

**Rule 3 — Tap-select at any level.** Children have higher z-index than ancestors on hit-test.
Transform (move/resize/rotate) of a selected child changes **its own-transform only**. Context
toolbar is for the selected child.

**Rule 4 — Freeform / marquee stay top-level.** Those gestures pick only top-level ink-boxes
(existing UX). Nested pick is tap (Rule 3).

**Rule 5 — Reparent at end of move.** After a move commits, new parent = the highest z-index
node that contains **80% of the moving node’s natural area**; else parent = document root.

## Resolution

**Adopted** — 2026-09-05 (PM). Human lock-in.

- Nested ink-boxes are **Must this campaign** under [REQ-05](../../../.docs/modules/epaper/prd.md#device-ink-box)
  and [REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation).
- [CHL-0011](../../iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md) future-intent is
  **scheduled** by this challenge (same outcome, plus Rules 1–5). Do not implement CHL-0011’s
  “TBD containment” — use Rule 5’s 80% natural-area bar and Rule 1 flatten.
- Not expedite. Feature is already in the execution lock (`epaper/ink-box`).
- **Needs design: no** for this slice — chrome inventory is unchanged (same overlay, same
  context toolbar). Hit-test and membership change. Dual-ask: `/qa` BDD from the new AC;
  Designer is not blocking.
- Rotation **UI** stays out ([REQ-08](../../../.docs/modules/epaper/prd.md#node-manipulation));
  Rule 1/2/3 may bake a reserved rotation field if it is already set.

## Product doc updates

- `.docs/modules/epaper/prd.md` — REQ-05 / REQ-06 / Non-Goals / Success Metrics
- `.docs/modules/epaper/features/ink-box/srs-product.md` — BR-B20…BR-B25
- `.docs/modules/epaper/features/ink-box/srs-experience.md` — nested tap + flatten journeys
- This file; CHL-0011 pointer to CHL-0032

Architect binds SRS-EP-10 / SRS-EP-11 (and new sections as needed), domain SmartGroup children,
and an ADR for RenderingContext.

**Amend 2026-09-05 (human).** Overflow of a nested child past the parent’s **natural world AABB**
is neither painted nor tap-selectable. Clip is that AABB (not even-odd boundary ink) so dirty/cull
stay O(1) per group. Bound in [SRS-EP-76](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-76-nested-render)
/ [SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)
and [ADR-0039](../../../.docs/adr/ADR-0039-nested-ink-box-rendering.md) §6.

## Interrupt / expedite (when applicable)

n/a
