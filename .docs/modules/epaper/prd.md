---
title: PRD — Epaper
module: epaper
version: 0.2.0
lifecycle: active
parent_brd: [BRD-06, BRD-07]
owner: pm
source: EXP-0001
---

# PRD — Epaper

Native drawing surface that runs **on the reMarkable 2**: pen ink renders locally
on the e-paper panel (not relayed via macOS). Sibling to desktop [Infini](../infini/prd.md);
code lives in repo-root `epaper/`.

## Problem & Job-to-be-Done

Artists and note-takers want the reMarkable’s writing feel while using a larger
synced canvas on the desktop. Relaying every pen sample to the desktop and pushing
pixels back is too slow for local ink. Epaper owns on-device ink and participates in
drawing-region sync with Infini.

## Success Metrics

| Metric | Target | Source |
|---|---|---|
| Pen-down → local pixel (p95) | ≤ 30 ms | Manual / EXP S1 |
| Orientation / aspect match | Circle stays circle; axes match landscape use | Manual |
| Drawing-region map latency | Next sample uses Infini viewport (ahead of full refresh) | Manual / EXP S3 |

## [REQ-01] Local pen-matched ink {#local-pen-ink}
- **Priority:** Must · **Traces:** [BRD-06]
- Needs design: no
- While Epaper is running (xochitl stopped), pen strokes appear on the RM2 panel
  under the pen tip with correct landscape orientation and aspect.

**Acceptance**
- Given Epaper is fullscreen on RM2, When the user draws a stroke, Then black ink
  appears along the pen path with p95 ≤30 ms pen-down → pixel (no Mac round-trip).
- Given landscape device use on a portrait panel, When the user draws a circle,
  Then the on-screen stroke is circular (not elliptical) and follows the pen
  direction.

## [REQ-02] Drawing-region sync with Infini {#region-sync}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- Epaper sends completed (or streaming) vectors to Infini and applies Infini’s
  drawing-region updates. Full e-paper refresh may lag; the coordinate mapping for
  new pen input must track Infini with least latency. Shared document content for the
  visible drawing region must not diverge from Infini.

**Acceptance**
- Given a live Infini session, When the user draws on Epaper, Then vectors reach Infini
  with p95 ≤50 ms after the sample.
- Given Infini changes pan/zoom, When Epaper receives the drawing region, Then the next
  pen sample (≤100 ms map apply p95) uses that world region even if the panel still
  shows a stale refresh (ghosting allowed).
- Given a refresh of the drawing region, When Epaper paints that region, Then it matches
  Infini’s document for the same bounds (0 divergent strokes).

---

## Non-Goals

- **On-device pan / zoom / pinch** on the Epaper UI (deferred — gesture detection and
  e-paper refresh are too slow for a good experience).
- Acting as a Reawa-style mouse/stylus driver for other Mac apps.
- Cloud sync or multi-peer sessions.

## Assumptions & Dependencies

- Infini [REQ-01]–[REQ-03] define the desktop side of the session.
- Consistency mechanism owned by Architect ([ADR-0009](../../adr/ADR-0009-shared-document-viewport.md)).

## Open Questions

- Stroke batching vs per-sample stream for transmit — **owner:** architect — **needed by:** 2026-08-17

## Linked Modules

- [infini](../infini/prd.md)
- [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
