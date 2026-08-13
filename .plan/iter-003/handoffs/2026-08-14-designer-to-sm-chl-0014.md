---
from: designer
to: sm
date: 2026-08-14
iter: iter-003
---

# Hand-off: Designer → SM

## Context

UI-EP-03 revised for [CHL-0014](../challenges/CHL-0014-selection-rect-and-freeform.md). Package: `.plan/iter-003/design/selection-enclose-chrome/`. New scene `selection-enclose-chrome-sel-lasso.html`. EP-022 still `done` with updated AC/scenes. EP-018 AC updated for both modes.

**Verdict:** READY-WITH-CONCERNS — confirm 64×64 latches under the chip (not in the chip) in the navigator.

## Asks

1. `/qa` refresh BDD if needed (already extended in `selection-create-surround.feature`).
2. Then `/dev` on EP-018.

## Constraints

- Do not implement a fourth ToolChip.
- Do not keep the lasso polyline after pen-up.

## Out of scope

- Nested enclose, undo chrome.
