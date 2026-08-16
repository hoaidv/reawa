---
id: CHL-0020
author: designer
target: [SRS-EP-12]
severity: low
status: resolved
resolution: adopted
opened: 2026-08-15
iter: iter-004
expedite: false
interrupts_track: ""
---

# CHL-0020 — Enclose blink vs membership highlight (no membership blink)

## Context

Blink-on-membership lagged Pen on continuous draw-into. Human revised UX 2026-08-15.

Spec: [UI-EP-06](../design/recog-blink/ui-spec.md).

## Proposal

- Keep `ovl.enclose_blink` (whole box, one 250 ms width pulse).
- Replace membership blink with `ovl.membership_highlight`: 2× boundary of the **last** joined box; clear when a stroke does not join that box; reset on tool change / undo / redo.
- Durable chrome state belongs in a later architect story (STORY-EP-032). This challenge is the product/UI contract only.

## Resolution

**Adopted 2026-08-16 (close-iter).** Membership blink replaced by last-join highlight in iter-004. Durable chrome owner remains [STORY-EP-032](../stories/STORY-EP-032.md) (`draft`, parked — not carried into empty iter-005).

## Product doc updates

UI-EP-06 / SRS-EP-12 inventory already recorded during the iter; no further SRS edit at close.
