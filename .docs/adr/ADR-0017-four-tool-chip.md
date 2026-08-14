---
id: ADR-0017
title: ToolChip: Selection rect, Selection freeform, Pen, Ink-box
status: superseded
date: 2026-08-14
deciders: [architect, pm]
supersedes: null
superseded-by: [ADR-0021]
amends: [ADR-0013, ADR-0016]
source: CHL-0015
---

# ADR-0017 — ToolChip four tools (two Selection arms)

## Context

[CHL-0014](../../.plan/iter-003/challenges/CHL-0014-selection-rect-and-freeform.md) adopted two
selection hit-tests (rect AABB vs freeform polyline). Placement was two extra latches beside a
single mouse-like `Selection` chip — a fourth *row* of chrome, not a fourth *tool*.

Human (2026-08-14): those modes **replace** the mouse-like button in the **primary** bar. No
separate sub-mode strip. Inventory is four exclusive tools:

**Selection rect | Selection freeform | Pen | Ink-box**

[ADR-0016](./ADR-0016-selection-create-enclose-cta.md) still forbids putting **Enclose** or **undo**
on the chip. This ADR only splits Selection.

Quality: one tap to arm the gesture (no extra latch); chip still partial-refresh; Enclose stays
contextual.

## Decision

1. ToolChip inventory is **four** exclusive tools: `sel_rect` · `sel_freeform` · `pen` · `ink_box`.
   Default remains `pen`. Same 64×64 tiles, same floating chip, hug width.
2. There is **no** `tool.selection` control and **no** `tgl.sel_*` off-chip.
3. `cta.enclose` stays on SelectionOverlay when **either** selection tool is armed and the
   selection is non-empty ([ADR-0016](./ADR-0016-selection-create-enclose-cta.md)).
4. Undo chrome later adopted as **actions** after a gap, not a fifth exclusive tool
   ([ADR-0018](./ADR-0018-undo-redo-chip-actions.md)). Enclose still must not join this row.

## Consequences

- [SRS-EP-04](../modules/epaper/features/tool-modes/srs-logic.md) / [SRS-EP-05](../modules/epaper/features/tool-modes/srs-ui.md)
  closed inventory is four tools. UI-EP-01 and UI-EP-03 compose the same chip.
- EP-018 implements two arming states, not a sub-mode flag.
- Chip is wider (4×64). Exclusion rect grows with hug width.

## Alternatives Considered

| Approach | Why rejected |
|---|---|
| Keep 3-chip + latches under chip (CHL-0014 placement) | Human: modes belong in the primary bar |
| Heuristic (straight drag vs lasso) without a control | Ambiguous mid-gesture; two explicit arms |
| Enclose as 5th chip | ADR-0016 / CHL-0010 |
