---
from: pm
to: sm
date: 2026-08-11
re: CHL-0004 Adopted — implement IN-023/024 EP-008/009
---

# Handoff — PM → SM (CHL-0004 adopted)

## Verdict

**Adopted** (already recorded on CHL-0004). Restores BR-09i / SRS-IN-11: boundary ink always transforms; `fixedInk` only exempts content UV sample size.

## Stories (Dev executing immediately per human)

| Story | Focus |
|---|---|
| STORY-IN-023 | fixedInk resize = scale+translate |
| STORY-IN-024 | desktop ink-scale mode toggle |
| STORY-EP-008 | tablet handle → tool_intent resize |
| STORY-EP-009 | move/resize ghost with ink preview |

## Notes

- No interrupt-track (severity normal; stays TRACK-003).
- Architect SRS amendments accepted as product truth.
- After Dev → `/qa` → human re-verify tablet resize, desktop boundary stretch, Fixed/Scale toggle, ink ghost.
