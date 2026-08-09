---
title: PRD — Epaper
module: epaper
version: 0.1.0
lifecycle: active
parent_brd: [BRD-06]
owner: pm
source: EXP-0001
---

# PRD — Epaper

Native drawing surface that runs **on the reMarkable 2**: pen ink renders locally
on the e-paper panel (not relayed via macOS). Sibling to the macOS Reawa module
(`Sources/ReawaApp`); code lives in repo-root `Epaper/`.

## Problem & Job-to-be-Done

Artists and note-takers want the reMarkable’s writing feel while using a larger
or synced canvas on the Mac. Relaying every pen sample to macOS and pushing pixels
back is too slow for local ink. Epaper owns on-device ink first; sync is a later
concern.

## Success Metrics

| Metric | Target | Source |
|---|---|---|
| Pen-down → local pixel (p95) | ≤ 30 ms | Manual / EXP S1 |
| Orientation / aspect match | Circle stays circle; axes match landscape use | Manual |

## [REQ-01] Local pen-matched ink {#local-pen-ink}
- **Priority:** Must · **Traces:** [BRD-06]
- While Epaper is running (xochitl stopped), pen strokes appear on the RM2 panel
  under the pen tip with correct landscape orientation and aspect.

**Acceptance**
- Given Epaper is fullscreen on RM2, When the user draws a stroke, Then black ink
  appears along the pen path without waiting for a Mac round-trip.
- Given landscape device use on a portrait panel, When the user draws a circle,
  Then the on-screen stroke is circular (not elliptical) and follows the pen
  direction.
