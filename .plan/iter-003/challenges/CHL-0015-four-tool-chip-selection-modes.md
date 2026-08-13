---
id: CHL-0015
title: Two Selection tools on the primary ToolChip (replace mouse-like)
author: pm
target: [REQ-03, REQ-05, SRS-EP-04, SRS-EP-05, SRS-EP-10, SRS-EP-12, ADR-0016, CHL-0014]
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
source: human product intent (ToolChip inventory)
---

# CHL-0015 — Four primary tools; no separate sub-mode strip

## Context

[CHL-0014](./CHL-0014-selection-rect-and-freeform.md) adopted rect vs freeform hit-tests but placed
them as latches **next to** a single Selection (mouse-like) chip.

## Proposal (human)

Replace the current mouse-like button. No separate place for sub-mode. Primary bar:

**Selection with Rect | Selection with Freeform | Pen | Ink-box** (four buttons).

## Resolution

**Adopted** 2026-08-14. [ADR-0017](../../../.docs/adr/ADR-0017-four-tool-chip.md) amends ADR-0013 /
ADR-0016. Hit-tests from CHL-0014 stand. Placement from CHL-0014 is **superseded**.

## Product doc updates

REQ-03, REQ-05, SRS-EP-04/05, SRS-EP-10/12, journeys, UI-EP-01, UI-EP-03, EP-018 AC.

## Interrupt / expedite

EP-018 waits on this design revision.
