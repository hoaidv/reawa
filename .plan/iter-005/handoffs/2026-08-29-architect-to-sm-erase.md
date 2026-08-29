---
from: architect
to: sm
date: 2026-08-29
iter: iter-005
---

# Hand-off: Architect → Scrum Master

## Context

REQ-11 bound as feature [erase](../../../.docs/modules/epaper/features/erase/index.md) — **one** [srs-logic.md](../../../.docs/modules/epaper/features/erase/srs-logic.md) (logic + quality; do not fork srs-ui):

| Id | Title |
|---|---|
| [SRS-EP-54](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-54-erase-mode) | Eraser mode, ToolChip, routing, barrel, nib |
| [SRS-EP-55](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-55-clip-remnants) | Geometric clip and remnant split |
| [SRS-EP-56](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-56-brush) | Brush erase |
| [SRS-EP-57](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-57-area) | Area erase |
| [SRS-EP-58](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-58-object) | Object erase and 80% table |
| [SRS-EP-59](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-59-erase-quality) | Erase quality |

Decision: [ADR-0034](../../../.docs/adr/ADR-0034-erase-clip-remnants.md) accepted. Path A/B: [SRS-EP-27](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md), [SRS-EP-28](../../../.docs/modules/epaper/features/device-document/srs-logic.md), [SRS-EP-29](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md), [SRS-EP-30](../../../.docs/modules/epaper/features/local-pen-ink/srs-quality.md) **retired**. Domain: boundary polyline on SmartGroup; barrel `toggle_pen_eraser` / `temp_erase` = last-used.

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Clip vs remnant vs 80% table separated; millimetre→du constants pinned (1 mm ≈ 8.90 du @ 226 dpi) |
| Concern | Hover circle kill-switch is product, not a second size |
| Concern | Hardware nib unverified — do not ship-gate on it |
| Risk | Clip precision vs 50 ms bar — measure on host; miss is a challenge, not sample-drop |

SRS orphans until code exist are expected.

## Asks

1. Implement stories only (human waived design package and Quality Assurance Engineer BDD).
2. Cancel [STORY-EP-040](../stories/STORY-EP-040.md), [STORY-EP-041](../stories/STORY-EP-041.md), [STORY-EP-042](../stories/STORY-EP-042.md).
3. Stop for human review before Developer execution.

## Constraints

- Suggested order: 54 (chip) ∥ 55 (engine) → 56 brush → 57 area → 58 object.
- Do not reopen TRACK-006. Do not implement clipboard in this slice.

## Out of scope

- Dimension type in `DocNode`. Size slider. Path B. Exclusive Hand mode.
