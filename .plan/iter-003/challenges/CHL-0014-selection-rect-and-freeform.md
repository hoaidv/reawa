---
id: CHL-0014
title: Two Selection sub-modes — rect and freeform
author: pm
target: [REQ-05, SRS-EP-10, SRS-EP-11, SRS-EP-12, STORY-EP-018, STORY-EP-022]
severity: high
status: resolved
resolution: adopted
resolved_by: pm
resolved: 2026-08-14
opened: 2026-08-14
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human product intent (Creation B select step)
---

# CHL-0014 — Two Selection sub-modes (rect + freeform)

## Context

Creation B select step was specified as a single AABB rubber-band ([CHL-0013](./CHL-0013-selection-create-feedback-enclose-cta.md)).
Human (2026-08-14) needs **two** ways to gather nodes before Enclose.

## Proposal (human)

1. **Selection rect** — drag; thin dotted rectangle while dragging and after pen-up until settled chrome; select whatever is **within that rectangle**; one straight drag.
2. **Selection freeform** — draw around; thin dotted polyline while drawing; pen-up **closes** the polyline; chrome becomes thin dotted **rectangle** (settled union AABB); select whatever is **inside the polyline**, not the rectangle.

## Resolution

**Adopted** 2026-08-14 (PM). Enclose CTA, 6 anchors, tight union AABB, no fourth ToolChip, no nesting — unchanged.

Locks:

- Two mutually exclusive **primary tools** (placement superseded 2026-08-14 by [CHL-0015](./CHL-0015-four-tool-chip-selection-modes.md) / [ADR-0017](../../../.docs/adr/ADR-0017-four-tool-chip.md)): `sel_rect` and `sel_freeform` on the ToolChip. Hit-tests below still stand.
- Rect hit-test = AABB intersect. Freeform hit-test = even-odd inside closed polyline (ink ≥80% samples; other nodes centroid).
- Settled chrome after **either** gesture = `ovl.nodes_bounds` + 6 anchors + `cta.enclose`. Freeform polyline is gone after pen-up.

## Product doc updates

- `.docs/modules/epaper/prd.md` REQ-05 Creation B
- `srs-logic.md` SRS-EP-10 select steps; SRS-EP-11 pickable sets
- `srs-ui.md` inventory, interaction, states (`sel.lasso`)
- `srs-experience.md` `journey.device_select_create`
- BDD `selection-create-surround.feature`
- Design UI-EP-03 revision (EP-022)

## Interrupt / expedite

Does not freeze TRACK-003. EP-018 implement waits on design revision of UI-EP-03.
