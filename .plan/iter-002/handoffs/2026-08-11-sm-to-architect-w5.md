---
from: sm
to: architect
iter: iter-002
date: 2026-08-11
subject: w5-viewport-live-thicken
cc: [qa, dev, pm]
---

# Handoff — W5 open: thicken for pan/zoom → tablet

## Context

W4 Must gated (READY-WITH-CONCERNS). Live EXP StrokeSync draw works RM→Infini, but
**ADR-0009 viewport → tablet refresh** is not wired. Human opened W5:

1. **Region marker** visible **only during** pan/zoom.
2. On pan/zoom, sync tablet-sync region (+ content if needed) to tablet.
3. Tablet refresh at **e-ink-safe rate** (coalesce; do not refresh every frame).
4. Goal: draw anywhere on infinity canvas via desktop pan/zoom.

## Stories (draft — hold `ready` until your thicken + QA BDD)

| Story | Module | Intent |
|---|---|---|
| [STORY-IN-011](../stories/STORY-IN-011.md) | Infini | Marker + coalesced viewport publish into session |
| [STORY-EP-002](../stories/STORY-EP-002.md) | Epaper | Immediate map apply + coalesced region refresh |

No design stories (REQ-03 Needs design: **no**). Marker = thin sync affordance.

## Thicken asks (required)

Update `tablet-sync` / `region-sync` SRS (logic + quality) so Dev/QA have closed rules:

| Topic | Ask |
|---|---|
| Marker | When visible / hide; relation to `drawingRegion` AABB; not permanent chrome |
| Viewport coalesce | Infini publish policy under gesture spam |
| E-ink refresh | Max refresh Hz / min interval; map-immediate vs paint-async (ADR-0009) |
| **Stroke scale** | World-unit stroke width; Desktop↔Tablet apparent size parity vs region; **zoom** must shrink/grow strokes on tablet like desktop |
| Content | When refresh must include doc ∩ region (same-picture) |

Human reports current stroke size looks inconsistent vs tablet-sync region; fix in contract
before implement.

## Out of scope this wave

- STORY-IN-010 Smart Group (Could, parked)
- DocChrome / STORY-IN-006
- Reconnect snapshot/hello (prior concern — still TBD)

## Next

1. **`/architect`** — thicken + review verdict → handoff SM/QA  
2. **`/qa`** — BDD for IN-011 / EP-002  
3. **`/dev`** — implement IN-011 then EP-002  
4. Human verify: pan/zoom moves region; draw anywhere; stroke scale matches
