---
from: sm
to: designer
iter: iter-003
date: 2026-08-11
subject: ink-box-ui-design-ready
cc: [pm, qa, dev]
verdict: READY
---

# SM → Designer — Infini ink-box design ready (W3)

## Pickup

| Story | Package | Status |
|---|---|---|
| [STORY-IN-013](../stories/STORY-IN-013.md) | `.plan/iter-003/design/ink-box-ui/` | **ready** |

## Must cover

- ToolStrip: Selection · Ink-box ([SRS-IN-14](../../../.docs/modules/infini/features/vector-document/srs-ui.md#srs-in-14-ink-box-ui))
- SelectionOverlay: bounds, resize handles (**no rotation**), `inkScaleMode` toggle
- **Refuse-create** state: `cta.create_smart_group` / `ind.create_refused_no_surround` (SRS-IN-16)
- Below-LOD manipulation unavailable

## Hold

- [STORY-EP-003](../stories/STORY-EP-003.md) stays **draft** until [STORY-EP-004](../stories/STORY-EP-004.md) spike completes.

## Parallel

`/qa`→`/dev` run IN-012 + EP-004 while you design — no file conflict with `design/ink-box-ui/`.
