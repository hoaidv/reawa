---
id: CHL-0011
title: Nested SmartGroup enclose — capture ink-boxes as content
author: pm
target: [REQ-05, SRS-EP-10]
severity: medium
status: resolved
resolution: adopted-future
resolved_by: pm
resolved: 2026-08-13
opened: 2026-08-13
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human product intent during EP-016 verify
---

# CHL-0011 — Nested SmartGroup enclose (ink-boxes inside ink-boxes)

## Context

Human (2026-08-13), after verifying flat enclose on RM2: a Smart Group should not apply only to
ink. Enclose must eventually capture **both free ink and existing Smart Groups**, so a creator
can form nested ink-boxes (an outer box whose content children include whole inner boxes).

Today [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) and
`walkInkCandidates` capture **top-level free Ink only**. Ink under a Smart Group is skipped;
**Smart Group nodes themselves are never capturable content**. That is correct for the current
campaign slice (flat enclose + draw-into membership) and must stay so until this challenge is
scheduled.

## Proposal

**Adopt as a future product feature** (not this campaign / not W10–W12):

1. Enclose content candidates expand to **free Ink ∪ SmartGroup** (hit-test / containment rule
   for a whole box TBD by architect — e.g. bounds-inside or sample proxy).
2. Nested create reparents captured Smart Groups as content children (tree nesting), with
   layoutOffset / transform rules that do not break `fixedInk` / `withBounds`.
3. Draw-into membership and manipulation of nested trees stay consistent with
   [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md).

Out of TRACK-003 current waves. Architect thickens when PM opens a later campaign slice.

## Resolution

**Adopted → future** — 2026-08-13 (PM).

- Capability is **accepted product intent**, not deferred-as-maybe and not rejected.
- **Not scheduled** on TRACK-003 W10–W12. Current enclose stays **flat: free ink only**.
- Product docs record the future outcome under epaper PRD Won't-this-campaign / Future, and
  SRS-EP-10 states the current capture set explicitly so Dev does not invent nesting mid-W10.
- No expedite. No interrupt. SM continues W10 membership ([STORY-EP-017](../stories/STORY-EP-017.md))
  without nesting scope.

## Product doc updates

- [epaper/prd.md](../../../.docs/modules/epaper/prd.md) — Future / Won't-this-campaign bullet for
  nested enclose
- [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) — capture inventory:
  free ink only this campaign; nesting → CHL-0011
- [srs-product.md](../../../.docs/modules/epaper/features/ink-box/srs-product.md) — edge case row
- [execution-board](../execution-board.md) backlog sink

## Interrupt / expedite (when applicable)

n/a
