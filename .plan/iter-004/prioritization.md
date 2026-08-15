---
iter: iter-004
author: pm
date: 2026-08-15
frameworks: [MoSCoW, RICE]
---

# Prioritisation — iter-004 (addendum: connector select + hand-touch)

> Addendum to the connector campaign. In the room: pm + human (2026-08-15). EP-029 verified.

## MoSCoW — this slice
| REQ | Title | Priority | Rationale |
|---|---|---|---|
| [REQ-09] select | Select recognized connector (rect/freeform + pen-hit) | Must | Chrome already assumes selected; gap is the pick grammar |
| [REQ-10] | Hand-touch first slice (hit box → freeform, finger move, no sub-64 gizmos) | Must | Start adopting finger on canvas; coarse only |
| Finger resize / pinch / pan | — | Won't | Fine chrome stays pen; pan/zoom still Non-Goal |

## RICE — sequence
| REQ | Reach | Impact | Confidence | Effort | Score | Rank |
|---|---|---|---|---|---|---|
| REQ-09 select | 8 | 2 | 0.9 | 2 | 7.2 | 1 |
| REQ-10 | 8 | 3 | 0.7 | 4 | 4.2 | 2 |

Connector select first: smaller, unblocks style/end chrome already designed. Hand-touch after.

## Decisions & cuts
- Primary finger-eligible size is **64 du**, not 32 (CHL-0019).
- 6 anchors (28/56) are pen-only.
- Finger empty-canvas does not lasso or switch tools.
