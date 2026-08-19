---
from: sm
to: designer
date: 2026-08-19
iter: iter-005
---

# Hand-off: SM → Designer — W1 EP-037 ∥ IN-034

Wave 0 Software Requirements Specification bind is **done** (architect verdict READY-WITH-CONCERNS). Two design stories are **ready**. Work-in-progress 2: **only these two packages**.

| Lane | Story | Package | Pri |
|---|---|---|---|
| A | [STORY-EP-037](../stories/STORY-EP-037.md) Design hand-touch: one-finger pick/move and two-finger pan/zoom | `design/hand-touch/` | P0 |
| B | [STORY-IN-034](../stories/STORY-IN-034.md) Design Infini pen-button map settings | `design/pen-button-map/` | P0 |

Queued (do not open): EP-040 erase · EP-043 clipboard · EP-045 ends · EP-048 attachments · EP-050 manual create.

## SRS parents (bound)

- EP-037 → [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) · [SRS-EP-22](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui) · [SRS-EP-23](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-23-finger-tool-switch) · [SRS-EP-24](../../../.docs/modules/epaper/features/region-sync/srs-logic.md#srs-ep-24-two-finger-viewport)
- IN-034 → [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) · [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) — **not** [SRS-IN-05](../../../.docs/modules/infini/features/vector-document/srs-ui.md)

## Experience thickness

Product Manager `srs-experience` / `srs-product` were **not** thickened in Wave 0 (architect Concern). For this wave, scene inventory = Product Requirements Document “UI states / journeys to design” **plus** the new `srs-ui` skeletons. Do not invent undeclared popups. Do not hard-stop the whole package for missing experience files; log the gap in the designer→SM handoff.

## Shared writes

Do **not** edit `.docs/design/index.md` or `.docs/DESIGN.md` — Scrum Master stitches after join. Shared `design/system/` : uniquely named files only.
