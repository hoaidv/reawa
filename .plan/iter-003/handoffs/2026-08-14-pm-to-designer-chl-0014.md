---
from: pm
to: designer
date: 2026-08-14
iter: iter-003
---

# Hand-off: PM → Designer

## Context

**Adopted [CHL-0014](.plan/iter-003/challenges/CHL-0014-selection-rect-and-freeform.md).** Creation B has two Selection **sub-modes** (not a fourth ToolChip). REQ-05, SRS-EP-10/11/12, journeys, and BDD updated.

## Asks

1. Extend UI-EP-03: `sel.marquee`, `sel.lasso`, settled `sel.nodes_selected`, `tgl.sel_rect` / `tgl.sel_freeform`.

## Constraints

- ToolChip stays Selection · Pen · Ink-box.
- Settled chrome after either gesture = tight union AABB + 6 anchors + Enclose.
- Freeform hit-test is inside the closed polyline, not the rectangle.

## Out of scope

- Nested enclose, undo chrome, EP-018 code until design lands.
