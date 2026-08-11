---
title: PRD — Epaper
module: epaper
version: 0.4.0
lifecycle: active
parent_brd: [BRD-06, BRD-07]
owner: pm
source: EXP-0001
---

# PRD — Epaper

Native drawing surface that runs **on the reMarkable 2**: pen ink renders locally
on the e-paper panel (not relayed via macOS). Sibling to desktop [Infini](../infini/prd.md);
code lives in repo-root `epaper/`.

**Docs rule (2026-08-11):** product docs describe **what ships in the Qt app today**.
The header-only `regionsync/` library is a tested target shape, not the device runtime.

## Problem & Job-to-be-Done

Artists and note-takers want the reMarkable’s writing feel while using a larger
synced canvas on the desktop. Relaying every pen sample to the desktop and pushing
pixels back is too slow for local ink. Epaper owns on-device ink and participates in
drawing-region sync with Infini.

## Success Metrics

| Metric | Target | Source |
|---|---|---|
| Pen-down → local pixel (p95) | ≤ 30 ms | Manual / EXP S1 |
| Orientation / aspect match | Circle stays circle; axes match chosen gut pose | Manual |
| Drawing-region map latency | Next sample uses Infini viewport (ahead of full refresh) | Manual / EXP S3 |
| Stroke thickness under zoom | Live + rasterized ink use world × panel scale | Manual |
| Tool switch (tap → active indicator) | p95 ≤ 300 ms; no measurable ink-latency regression | Manual |

## [REQ-01] Local pen-matched ink {#local-pen-ink}
- **Priority:** Must · **Traces:** [BRD-06]
- Needs design: no
- While Epaper is running (xochitl stopped), pen strokes appear on the RM2 panel
  under the pen tip with correct aspect. Digitizer→panel uses the verified Round 19
  map (landscape digitizer on portrait framebuffer). Infini gut orientation does **not**
  change this local map — it only changes sync-frame UV for world mapping.

**Acceptance**
- Given Epaper is fullscreen on RM2, When the user draws a stroke, Then black ink
  appears along the pen path with p95 ≤30 ms pen-down → pixel (no Mac round-trip).
- Given landscape device use on a portrait panel, When the user draws a circle,
  Then the on-screen stroke is circular (not elliptical) and follows the pen
  direction.

## [REQ-02] Drawing-region sync with Infini {#region-sync}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- Epaper streams panel-space stroke samples to Infini (`stroke_*`) and applies Infini’s
  `viewport` (drawing region + gut `orientation` + optional `settle`) and one-shot
  vector `doc_snapshot`. Full e-paper refresh may lag; coordinate mapping for new pen
  input must track Infini with least latency. PNG `region_refresh` is **not** used.

**Acceptance**
- Given a live Infini session, When the user draws on Epaper, Then stroke samples reach
  Infini with p95 ≤50 ms after the sample.
- Given Infini changes pan/zoom, When Epaper receives the drawing region, Then the next
  pen sample (≤100 ms map apply p95) uses that world region even if the panel still
  shows a stale refresh (ghosting allowed).
- Given a `doc_snapshot` or settle viewport, When Epaper paints the region, Then figures
  match Infini’s WorldLayer for the same bounds after sharp settle.
- Given zoom-out on Infini, When the user continues drawing, Then live stroke thickness
  matches thinned existing vectors (world width × current panel scale).

## [REQ-03] On-device tool modes {#tool-modes}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- The creator decides **on the device** what the pen does, without reaching for the desktop.
  A minimal, always-visible toolbar offers exactly three tools — **Selection · Pen · Ink-box** —
  switched by **finger touch**, so the pen stays free for content. `Pen` is the default and
  leaves [REQ-01](#local-pen-ink) local ink behaviour unchanged.
- **Ink-box** arms the next stroke as an *enclose request*: the stroke still streams as ordinary
  ink samples, and **Infini** performs recognition and grouping
  ([infini REQ-04](../infini/prd.md#smart-group)). Epaper performs no geometry recognition and
  holds no document tree.
- **Selection** lets the creator pick the object under the pen and move or resize it; the
  resulting document change is owned and applied by Infini.
- Tool state is **local to the device** — Infini neither drives nor mirrors it.

**Acceptance**
- Given Epaper is running, When the creator taps a tool with a finger, Then the toolbar shows the
  new active tool with p95 ≤300 ms and the pen's next action uses that tool.
- Given the `Pen` tool, When the creator draws, Then local ink latency stays within
  [REQ-01](#local-pen-ink) (p95 ≤30 ms pen-down → pixel) — the toolbar costs no ink latency.
- Given the `Ink-box` tool, When the creator draws an enclosing stroke, Then Infini receives the
  samples marked as an enclose request, and the resulting Smart Group appears on the panel after
  settle with p95 ≤500 ms after the op.
- Given any tool, When the pen passes over the toolbar strip, Then no ink is drawn there
  (0 stray strokes on the chrome region).
- Given a full-panel refresh is in flight, When the creator switches tools, Then the active-tool
  indicator is still legible (partial refresh of the strip, no dependence on the settled frame).
- **UI states / journeys to design:** default `Pen`; switching tools; `Selection` with nothing
  selected; `Selection` with a Smart Group selected (handles); `Ink-box` armed; enclose rejected
  (too small / no ink inside); toolbar during a trailing panel refresh.

---

## Non-Goals

- **On-device pan / zoom / pinch** on the Epaper UI (deferred).
- Acting as a Reawa-style mouse/stylus driver for other Mac apps.
- Cloud sync or multi-peer sessions.
- **Smart Group recognition / geometry on-device** — Epaper contributes tool intent and pen
  samples only; Infini recognizes, groups, and owns the tree ([REQ-03](#tool-modes)).
- A general on-device tool palette — the toolbar is exactly the three tools in
  [REQ-03](#tool-modes); no brushes, colors, layers, or document browser.
- Production use of `regionsync/` `append_ink` NetSink until wired into the Qt binary
  (library remains the future ADR-0009 shape).

## Assumptions & Dependencies

- Infini [REQ-01]–[REQ-03] define the desktop side of the session.
- Consistency: [ADR-0009](../../adr/ADR-0009-shared-document-viewport.md) (target) with
  interim `doc_snapshot` + `stroke_*` per architect amendment.
- Stroke paint: [ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md).

## Open Questions

- Wire Qt app to `regionsync/` / `doc_op` vs keep `stroke_*` — **owner:** architect
- Soft (interim) vs sharp settle visual quality on e-ink — **owner:** qa / human
- Is the RM2 capacitive touch surface usable from the Qt app — **owner:** architect —
  **needed by:** before the first [REQ-03](#tool-modes) story. The app handles pen events only
  today (`tabletappfilter`); if touch is unavailable, the fallback (pen-on-toolbar or a hardware
  button) changes the design.
- Does `Selection` pick on device — **owner:** architect — **needed by:** before the first
  [REQ-03](#tool-modes) story. Hit-test against the last `doc_snapshot` locally, or relay the
  pick to Infini.

## Linked Modules

- [infini](../infini/prd.md)
- [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
