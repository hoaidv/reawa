---
from: sm
to: designer
date: 2026-08-14
iter: iter-004
---

# Hand-off: SM → Designer — W1 ToolChip + connector chrome

## Context

TRACK-004 is **active**. Lock: **horizontal · `design-validated`**. Implement is sliced and frozen.

Pickup **in parallel** (different write sets):

| Story | Package | Binding |
|---|---|---|
| [STORY-EP-026](../stories/STORY-EP-026.md) **ready** | `.plan/iter-004/design/toolchip-recognizers/` | [SRS-EP-05](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md) · [ADR-0021](../../../.docs/adr/ADR-0021-connector-toolchip.md) · [REQ-03](../../../.docs/modules/epaper/prd.md#tool-modes) |
| [STORY-EP-027](../stories/STORY-EP-027.md) **ready** | `.plan/iter-004/design/connector-chrome/` | [SRS-EP-19](../../../.docs/modules/epaper/features/connector-ink/srs-ui.md) · [REQ-09](../../../.docs/modules/epaper/prd.md#device-connectors) |

Board: [execution-board](../execution-board.md). Track cursor: EP-026 ∥ EP-027.

## Asks

1. **EP-026** — paint the primary chip as:

   **Rect selection | Freeform selection | Pen ⟨space⟩ Ink-box recognizer | Connector recognizer ⟨space⟩ Undo | Redo**

   Scenes must cover: pen + both armed; a toggle off; Selection + dimmed toggles; undo empty no-op.
   Do-not-regress EP-023 / EP-025 (floating 32 px chip, 1-bit, partial refresh).

2. **EP-027** — create blink is **one-shot** of connector **+ both nodes**; do not name Ink/Curve at
   create time. Selection chrome: Ink|Curve and per-end Edge|Centre. `conn.rejected` = no banner.

3. Run `ui-spec-gate`. Copy `ui_spec` / `scenes` / `hifi` onto EP-028 / EP-030 when done.

## Constraints

- Platform is reMarkable 2 e-ink: 1-bit, no hover, no motion, no color.
- Default routing is **already decided** (Ink/Curve from the rest spine) — do not add a draw-time picker.
- Do not start implement UI stories. Do not edit `.docs/modules/**` except via a challenge.

## Out of scope

REQ-08, CHL-0011, CHL-0012, arrowheads, squared routing, desktop authoring.

## Next after you

Human types `/qa` only after PM `validated_by` (or, in parallel, EXP-0002 guard corpus as explore —
that is a **ship** gate, not this wave). Then `/dev`.
