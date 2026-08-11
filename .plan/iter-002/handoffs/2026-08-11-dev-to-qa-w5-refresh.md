---
from: dev
to: qa
iter: iter-002
date: 2026-08-11
subject: w5-refresh-wire-in-review
cc: [sm, pm]
---

# Dev → QA — tablet region refresh wired (follow-up)

## Bug fixed

Tablet content was not refreshing because viewport/publish stayed on `MemoryTransport`
and Epaper never received Infini→device messages.

## Shipped

| Area | Change |
|---|---|
| Infini Electron | Bidirectional TCP: `sendToRm` viewport + `region_refresh` PNG to RM clients |
| Infini Canvas | Capture sync-frame bitmap on pan/zoom settle (≥250 ms); max-fit centered frame; **Sync: Portrait/Landscape** toggle |
| Epaper | `StrokeSync` parses host lines; applies `region_refresh` to panel; orientation drives pen map |

## Deploy

- Epaper rebuilt + deployed (`RM_SYNC_HOST=10.11.99.12`)
- Infini `electron:dev` started for test

## How to verify

1. Pan/zoom on Infini → marker shows; tablet panel updates with content under the frame (after settle ~100–250 ms).
2. Toggle **Sync: Portrait / Landscape** → frame aspect flips; tablet refresh rotates accordingly.
3. Draw on RM → strokes still appear on Infini in the sync region.
