---
id: CHL-0020
author: designer
target: [SRS-EP-12]
severity: low
status: open
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
<!-- PM fills after triage -->

## Product doc updates
<!-- SRS-EP-12 inventory when adopted -->
