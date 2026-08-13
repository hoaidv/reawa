---
from: designer
to: sm
date: 2026-08-14
iter: iter-003
cc: [qa, dev]
---

# Hand-off: Designer → SM — EP-022 done

## Package

[UI-EP-03](../design/selection-enclose-chrome/ui-spec.md) —
`.plan/iter-003/design/selection-enclose-chrome/`

| Scene | State |
|---|---|
| `selection-enclose-chrome-sel-marquee.html` | `sel.marquee` |
| `selection-enclose-chrome-sel-nodes-selected.html` | `sel.nodes_selected` (hifi) |
| `selection-enclose-chrome-sel-create-refused.html` | `sel.create_refused` |
| `selection-enclose-chrome-sel-none.html` | `sel.none` |

6 anchors: **nw n ne sw s se** (no e/w). `cta.enclose` on overlay. ToolChip stays 3.

## Story

[STORY-EP-022](../stories/STORY-EP-022.md) **done**. Links copied to [STORY-EP-018](../stories/STORY-EP-018.md) **ready**.

## Gate

`ui-spec-gate` checklist: SRS ready, platform epaper, tokens.css+common.css, iframe index, press invert, icon files on disk. Scene graph N/A (in-scene).
