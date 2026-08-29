---
from: sm
to: human
date: 2026-08-29
iter: iter-005
---

# Hand-off: Scrum Master → Human (review implement stories)

## Context

Erase product locked and bound. **Developer has not started.** Review the five **draft** implement stories, then say `/dev` (or name which story) when ready.

Icons (no other design artifacts):

- `.docs/design/system/assets/icon-epaper-erase-brush.svg`
- `.docs/design/system/assets/icon-epaper-erase-area.svg`
- `.docs/design/system/assets/icon-epaper-erase-object.svg`

## Stories to review

| Id | Title | Parent specification | Depends on | Pts |
|---|---|---|---|---|
| [STORY-EP-062](../stories/STORY-EP-062.md) | Eraser mode, ToolChip, barrel last-used | [SRS-EP-54](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-54-erase-mode) | — | 5 |
| [STORY-EP-063](../stories/STORY-EP-063.md) | Geometric clip, remnant split, boundary polyline | [SRS-EP-55](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-55-clip-remnants) | — | 8 |
| [STORY-EP-064](../stories/STORY-EP-064.md) | Brush erase capsule clip | [SRS-EP-56](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-56-brush) | EP-062, EP-063 | 5 |
| [STORY-EP-065](../stories/STORY-EP-065.md) | Area erase clip and fully-inside remove | [SRS-EP-57](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-57-area) | EP-062, EP-063 | 5 |
| [STORY-EP-066](../stories/STORY-EP-066.md) | Object erase 80 percent table | [SRS-EP-58](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-58-object) | EP-062 | 5 |

Cancelled Path A/B: [STORY-EP-040](../stories/STORY-EP-040.md), [STORY-EP-041](../stories/STORY-EP-041.md), [STORY-EP-042](../stories/STORY-EP-042.md).

Human is Quality Assurance this wave — no behaviour-driven scenario gate.

## Asks

1. Read the five stories (and [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md) if a line looks wrong).
2. Edit stories in place, or say what to change.
3. When the slice is right: `/dev` — Scrum Master will not start implementation until then.

## Constraints

- Clipboard, Device Settings implement, connector ends stay queued.
- TRACK-006 stays closed.

## Out of scope

- `/dev` this turn.
