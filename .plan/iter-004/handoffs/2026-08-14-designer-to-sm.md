---
from: designer
to: sm
date: 2026-08-14
iter: iter-004
---

# Hand-off: Designer → SM — EP-026 + EP-027 done

## Context

W1 design packages are painted. Lock remains vertical / `verified`. Implement stays `draft` until `/qa` BDD.

## Delivered

| Story | Package | Spec | Hifi |
|---|---|---|---|
| [STORY-EP-026](../stories/STORY-EP-026.md) **done** | [toolchip-recognizers](../design/toolchip-recognizers/) | [UI-EP-04](../design/toolchip-recognizers/ui-spec.md) | `toolchip-recognizers-pen-armed.html` |
| [STORY-EP-027](../stories/STORY-EP-027.md) **done** | [connector-chrome](../design/connector-chrome/) | [UI-EP-05](../design/connector-chrome/ui-spec.md) | `connector-chrome-blink.html` |

Open each package `index.html` (iframe navigator, RM2 landscape @ 100%).

**Binding**

- Chip: **Rect | Freeform | Pen ⟨32 px⟩ Ink-box recog | Connector recog ⟨32 px⟩ Undo | Redo**
- Tiles **64×64** (shipped EP-023), not SRS 32 px — [CHL-0019](../challenges/CHL-0019-toolchip-tile-size.md) for `/pm`
- Blink: one Mono pulse (~250 ms), invert connector + both nodes, **no Ink/Curve name**
- Selected: Ink\|Curve + per-end Edge\|Centre; rejected = no banner

Copied `ui_spec` / `scenes` / `hifi` onto EP-028, EP-030, EP-031.

## Asks

1. `/pm` triage CHL-0019 (amend SRS-EP-05 tile size).
2. `/qa` BDD, then `/dev` EP-028 (depends EP-026 done).

## Constraints

Do not treat HTML as production QML. 1-bit, no hover/focus/motion.

## Out of scope

REQ-08, arrowheads, draw-time routing picker.
