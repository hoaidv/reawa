---
from: sm
to: qa
date: 2026-08-16
iter: iter-004
cc: [dev, pm]
---

# Hand-off: SM → QA — EP-034 still NOW; two new bugs queued

## Context

Human reported two defects while reviewing IN-030 / EP-034.

**Bug 1 — Infini demo mix** → [STORY-IN-032](../stories/STORY-IN-032.md) `ready`.
Cmd+R / startup always paints `demoPrimitives()` (line, circle, rect, zigzag). RM ink then either replaces them or **stacks on top**. Cause: `CanvasStage populated` + `paintMirror` re-seed when the mirror is empty.

**Bug 2 — USB plugged, ping timeout** → [STORY-EP-036](../stories/STORY-EP-036.md) `ready`, `depends_on` EP-034.
Same class as xochitl Help omitting `10.11.99.1`. Not EP-034 (that is ping-**alive**, Infini down). Want detect **on epaper**, restore gadget without unplug, then StrokeSync 2s retry.

## Asks

1. `/qa` [STORY-EP-034](../stories/STORY-EP-034.md) first (already `in-review`).
2. Do not start IN-032 / EP-036 until EP-034 scores; then `/dev` IN-032 (Infini-only) can ∥ EP-036 (epaper gadget) under WIP 2.

## Constraints

Do not unplug for ping-alive Infini-down. Do not treat demo figures as device connectors.

## Out of scope

Iter-005 slice. REQ-08.
