---
id: CHL-0028
author: pm
target: [REQ-11]
severity: high
status: adopted
opened: 2026-08-29
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0028 — Three exclusive erasers replace Path A / Path B

## Context

Human unlocked W3 erase and replaced the 2026-08-16 [REQ-11](../../../.docs/modules/epaper/prd.md#erase) Path A (nib sample-delete, 0 new Ink nodes) / Path B (erase selected nodes) draft. Product lock: [eraser-product.md](../eraser-product.md) (human-edited 2026-08-29). Chip grows three exclusives; remnant split; object 80% table; Path B retired.

## Proposal

Adopt the lock as the **single** product specification [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md). REQ-11 / REQ-03 / REQ-18 point at it; do not fork into srs-product / srs-experience / srs-ui from Product Manager. Architect binds one feature SRS + ADR.

## Resolution

**Adopted** 2026-08-29 (pm). Human: promote lock as PRD, do not scatter; UI/UX already in that document.

## Product doc updates

- `.docs/modules/epaper/prd-erase.md` — canonical REQ-11 specification
- `.docs/modules/epaper/prd.md` — REQ-11 / REQ-03 / REQ-18 stubs + success metric
- SRS-EP-27 / SRS-EP-28 / SRS-EP-29 Path A/B — retired by Architect (superseded-by new erase SRS)
- [STORY-EP-040](../stories/STORY-EP-040.md) design chrome package — cancelled (icons only this wave)
- [STORY-EP-041](../stories/STORY-EP-041.md) Path A nib sample-delete — cancelled
- [STORY-EP-042](../stories/STORY-EP-042.md) Path B — cancelled
