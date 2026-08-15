---
from: sm
to: architect
date: 2026-08-15
iter: iter-004
---

# Handoff — SM → Architect (parked)

**Do not start now.** Campaign cursor is **`/dev` EP-030**.

[STORY-EP-032](../stories/STORY-EP-032.md) is **draft** on TRACK-004: model device UI chrome (enclose blink, last-join membership highlight, selection overlay, ToolChip) as one state owner. Interim UX is already specified in [UI-EP-06](../design/recog-blink/ui-spec.md) / [CHL-0020](../challenges/CHL-0020-recog-width-blink.md). `/dev` implemented the stopgap on `TabletCanvasItem` flags so Pen is not starved.

When you pick this up: ADR first, then `/dev` implements EP-032. Do not silently replace the highlight contract.

Next for the campaign: **`/dev` EP-030**. Later: **`/architect` EP-032**.
