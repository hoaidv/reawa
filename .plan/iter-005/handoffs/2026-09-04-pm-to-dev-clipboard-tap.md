---
from: pm
to: dev
date: 2026-09-04
iter: iter-005
---

# Hand-off: PM → Dev

## Context

Adopted [CHL-0031](../challenges/CHL-0031-clipboard-tap-paste.md). Paste origin is the last **tap**,
not long-press. Device bugs: paste free ink into an ink-box failed (`insertUnder` rejects
SmartGroup); copy of a tap-selected ink-box dropped children.

## Asks

1. [STORY-EP-044](../stories/STORY-EP-044.md) — retarget to tap-origin paste; merge
   `clipboard_ops.hpp` into `clipboard.hpp`.
2. Keep tap vs travel (1 mm). Remove the 500 ms hold menu from InputHub.
3. `/qa` after tests pass.

## Constraints

- Do not put copy/cut/paste on `DocContext`.
- 0 `doc_change` for cut/paste this track.
- Paste visible iff slot non-empty **and** a tap location exists.
