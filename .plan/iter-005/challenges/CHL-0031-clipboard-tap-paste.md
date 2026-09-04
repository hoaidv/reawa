---
id: CHL-0031
author: pm
target: [SRS-EP-31, SRS-EP-32, SRS-EP-73]
severity: high
status: adopted
opened: 2026-09-04
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0031 — Clipboard paste origin is tap, not long-press

## Context

Device verify of STORY-EP-044 (2026-09-04):

- Cannot paste free ink into an ink-box (`insertUnder` rejects SmartGroup as `bad_parent`).
- Paste onto empty canvas works.
- Copy/paste of a selected ink-box fails (tap-select only has the SmartGroup id, so clone grain dropped all children).
- Long-press chrome worked and did not steal other tools — kept as a positive, but the creator now wants paste on the **normal** context toolbar.

Human lock-in: no long-press paste. Paste is visible when the slot is non-empty **and** there is a tap location (tap empty or tap a node). Freeform / marquee have no tap location → no paste button.

## Proposal

Adopt. Update clipboard product + SRS. Paste origin = tap world point. Clone a selected root in full unless some descendants are also selected. Parenting into a SmartGroup uses `insertAt` + join-style local ink.

## Resolution

resolution: adopted

## Product doc updates

- `.docs/modules/epaper/features/clipboard/srs-product.md`
- `.docs/modules/epaper/prd.md` REQ-12
- SRS-EP-31 / SRS-EP-32 / SRS-EP-33 / SRS-EP-11 hold-still
- ADR-0037 amendment
- `clipboard.feature`
- STORY-EP-044 AC
