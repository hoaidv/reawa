---
from: sm
to: dev
date: 2026-08-15
iter: iter-004
---

# Handoff — membership highlight (stopgap)

[UI-EP-06](../design/recog-blink/ui-spec.md): membership **highlight** last-join parent boundary (no blink). Enclose still one-shot blink. Reset highlight on exclusive tool change / undo / redo.

Implemented on `TabletCanvasItem` (`m_highlightInkIds`). Do not add a membership timer.

Campaign next remains [STORY-EP-030](../stories/STORY-EP-030.md). Chrome state machine is [STORY-EP-032](../stories/STORY-EP-032.md) for `/architect` later.
