---
from: sm
to: human
date: 2026-08-31
iter: iter-005
---

# Hand-off: Scrum Master → human — field latency stories ready

## Context

[STORY-EP-069](../stories/STORY-EP-069.md) (ToolContextImpl host ports and SelectionOverlay) is **done**. `epaper/drawing/tools/ui/selection_overlay.cpp` is that delivery (host-owned rename of ToolChrome), human-verified 2026-08-31 — not leftover work.

Camera blit + LatestJob pipeline: human said **better now**. Project memory:

- [.docs/memory/camera-pan-zoom-rasterize.md](../../../.docs/memory/camera-pan-zoom-rasterize.md)
- [.docs/memory/ink-path-density-hitch.md](../../../.docs/memory/ink-path-density-hitch.md)

Three field follow-ups are **ready** on [TRACK-005](../../tracks/TRACK-005-hand-on-paper.md), wave **W-field-latency**. Clipboard and Device Settings stay frozen.

## Asks

1. Pick which Developer lane to start (work-in-progress 2). Do **not** run [STORY-EP-070](../stories/STORY-EP-070.md) and [STORY-EP-072](../stories/STORY-EP-072.md) in parallel — both write `tabletcanvasitem.cpp`.
2. Say **go** in this Scrum Master chat to spawn a Developer on the chosen story or pair (070+071 or 071+072).

## Constraints

- Human is Quality Assurance Engineer on device. No design package. No behavior-driven ceremony.
- Do not FullClear ordinary ink ([CHL-0029](../challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)).
- Do not put camera vector FullClear back on the pointer stack.
- Do not merge SelectionOverlay into ToolCanvasItem.

## Out of scope

- Clipboard / Device Settings
- TRACK-006 reopen
- Infini undo apply ([STORY-IN-038](../stories/STORY-IN-038.md) cancelled)
