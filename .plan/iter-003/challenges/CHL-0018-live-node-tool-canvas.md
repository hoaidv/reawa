---
id: CHL-0018
title: Live move/resize paints the node on ToolCanvasLayer (option 1)
author: pm
target: [REQ-06, SRS-EP-11, SRS-EP-12, SRS-EP-14, ADR-0019]
severity: medium
status: resolved
resolution: adopted
resolved_by: pm
resolved: 2026-08-14
opened: 2026-08-14
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human — EP-025 option-1 RM2 verify
---

# CHL-0018 — Live SmartGroup pixels on ToolCanvasLayer

## Context

[CHL-0017](./CHL-0017-selection-chrome-layers.md) / [ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md)
split chrome by refresh class. Human then chose how **the moving box itself** is painted during
move/resize:

| Option | During the gesture | Pen-up |
|---|---|---|
| **1 (adopted)** | Hide the origin box on CanvasLayer; paint the live node + AABB + handles on **ToolCanvasLayer** | Rasterize the committed node back onto CanvasLayer — one settled picture |
| **2 (not now)** | Keep painting the live node on CanvasLayer; ToolCanvasLayer is chrome-only | Same commit |

RM2 (2026-08-14): option 1 **works**. Move/resize on a separate layer is clean at pen-up. Mid-gesture
**e-ink ghosting and dirty traces still happen** — accepted for this campaign (BR-B15: slow is OK,
wrong is not). Option 2 is kept as a later consideration if CanvasLayer/ToolCanvas composition
causes rendering issues in a following phase.

## Proposal

Lock option 1 in the SmartGroup (ink-box) PRD and feature SRS. Do not slice option 2 in TRACK-003.

## Resolution

**Adopted** 2026-08-14 by PM (human-directed).

- Document truth is unchanged: the tree still transforms live; commit still equals last preview
  ([REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation) / BR-B10).
- **Paint** of that live node during the gesture is ToolCanvasLayer, not a second document blit
  and not an advisory ghost.
- Ghosting / dirty traces **during** the drag are a refresh allowance, not a product defect, as
  long as the settled frame matches the document.
- **Option 2 deferred** — not rejected. Reopen if a later rendering phase cannot isolate Pen ink
  from Mono chrome.

Not an interrupt. EP-025 stays the implement vehicle.

## Product doc updates

- Epaper [REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation)
- [ink-box srs-product](../../../.docs/modules/epaper/features/ink-box/srs-product.md) BR-B19
- [srs-ui](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) composition (SRS-EP-12)
- [srs-logic](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) live-manip paint row (SRS-EP-11)
- [srs-quality](../../../.docs/modules/epaper/features/ink-box/srs-quality.md) (SRS-EP-14)
- [epaper architecture](../../../.docs/modules/epaper/architecture.md) refresh note

Architect: amend [ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md) to record
option 1 vs option 2.

## Interrupt / expedite

Not an interrupt. TRACK-003 continues on EP-025 verify.
