---
from: pm
to: designer
date: 2026-08-14
iter: iter-003
cc: [sm, architect]
---

# Hand-off: PM — tight `sel.nodes_selected` rect

## Decision

`ovl.nodes_bounds` **tightly** equals the union of selected nodes’ world AABBs.
**0 extra padding.** Empty space inside the rect reads as more selected than there is.

PRD REQ-05 AC, SRS-EP-10/12, journey, BDD updated.

## Designer (same turn)

Human also: context buttons (Enclose) **icon-only**, size = primary ToolChip (64×64), **no**
context-toolbar chrome.

## Verdict

READY for design revision of UI-EP-03. EP-018 still `ready` — Dev must use tight AABB + icon CTA.
