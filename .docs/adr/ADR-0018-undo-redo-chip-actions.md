---
id: ADR-0018
title: ToolChip history actions: Undo and Redo
status: accepted
date: 2026-08-14
deciders: [architect, pm, human]
supersedes: null
amends: [ADR-0013, ADR-0016, ADR-0017]
source: CHL-0016
---

# ADR-0018 — ToolChip history actions: Undo and Redo

## Context

[ADR-0017](./ADR-0017-four-tool-chip.md) closed exclusive tools at four:
`sel_rect` · `sel_freeform` · `pen` · `ink_box`. [CHL-0010](../../.plan/iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md)
deferred undo chrome so those four would not become five drawing tools.

Human layout (2026-08-14 / [CHL-0016](../../.plan/iter-003/challenges/CHL-0016-undo-redo-toolbar.md)):

**Selection Rect | Selection Freeform | Pen | Ink-box | ⟨space⟩ | Undo | Redo**

Quality: recoverability without a keyboard; 1-bit partial-refresh of a still-small chip; Enclose
stays contextual ([ADR-0016](./ADR-0016-selection-create-enclose-cta.md)).

## Decision

1. The floating strip has **two clusters** and a **32 du gap**: four exclusive tools, then Undo and
   Redo as **actions** (`cta.undo`, `cta.redo`). Actions never become `toolMode`. Default tool remains `pen`.
2. Tiles stay **64×64**. Gap is not a fifth tool; it is still inside the ink-exclusion union.
3. **Redo** stores the forward counterparts just inverted (inverse-of-the-inverse), depth 20,
   beside the undo ring ([ADR-0032](./ADR-0032-inverse-op-undo.md) §5). A successful structural
   commit **clears** redo. A successful undo that applied ≥1 inverse pushes one redo entry. Skip and
   pure no-op undo do **not** push redo. `doc_load` clears both. Mid-gesture undo/redo latch; last
   request wins if both are tapped in flight.
4. Empty undo or redo is a **no-op**. Publish undo/redo as the counterpart op or `compound`, never
   `restore_snapshot` ([ADR-0032](./ADR-0032-inverse-op-undo.md) §4).

## Consequences

- Chip hug-width grows: `4×64 + 32 + 2×64` = 416 du. Exclusion rect grows with it.
- [SRS-EP-07](../modules/epaper/features/device-document/srs-logic.md) Redo row is no longer out of
  scope. Product “Redo out of scope” rows are revised to “branching / non-linear history still out”.
- Enclose is still not an exclusive tool.

## Alternatives Considered

| Approach | Why rejected |
|---|---|
| Fifth exclusive tool for undo | Arms undo instead of pen; contradicts “action not tool” |
| Hardware / two-finger only (CHL-0010) | Human asked for on-panel buttons |
| Undo without redo | Human specified both |
| Properties panel / extra row | Banned by SRS-EP-12 scarce chrome |
