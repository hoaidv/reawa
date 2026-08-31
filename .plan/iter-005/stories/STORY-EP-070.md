---
id: STORY-EP-070
title: Residual pen-to-ink lag on moderately dense pages
kind: implement
parent_srs: [SRS-EP-01, SRS-EP-03]
parent_req: [REQ-01]
status: ready
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a page with about four handwriting sentences plus some ink-boxes (not a packed dense page), When the creator draws ordinary free ink, Then /tmp/epaper-ink-path.log attributes every noticeable pen-to-ink gap (behind=, slowest=, nested spans=) so the steal is named, not guessed."
  - "Given that same page, When InkStrokeOperation callbacks stay under a few milliseconds, Then any remaining lag is not blamed on the stroke operation; the log must show the queued GUI work (LatestJob deliver, full-panel update, overlay paint, ingest, or other)."
  - "Given an attributed steal on that page, When the Developer removes it from the pen sample path, Then random small pen-to-ink gaps are gone or the remaining gap is e-ink refresh only (no behind=rasterizeVectors / toolPaint / LatestJob apply on down)."
  - "Given ordinary RecogOutcome::Ink, When this story lands, Then the panel still does not FullClear the document on that pen-up ([CHL-0029](../challenges/CHL-0029-settle-is-not-fullclear-on-ink.md))."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-070 — Residual pen-to-ink lag on moderately dense pages

Field 2026-08-31 after the blit + LatestJob camera pipeline: pan/zoom and the old dense-page hitch
are **better**, but pen-to-ink still shows **small, random, noticeable** lag on a **moderately**
dense page (about four sentences plus some ink-boxes). `InkStrokeOperation` was already cheap;
the remaining steal is still on the GUI thread after the sample.

Canonical: [SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md#srs-ep-01-pen-event-path-coordinate-map-and-pen-mode-refresh)
(do not steal the GUI thread); [CHL-0029](../challenges/CHL-0029-settle-is-not-fullclear-on-ink.md);
probe [memory/ink-path-density-hitch.md](../../../.docs/memory/ink-path-density-hitch.md).

Human is Quality Assurance Engineer on device. No design package. No behavior-driven ceremony.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Field

| | |
|---|---|
| Page | ~4 handwriting sentences + some ink-boxes (not packed) |
| Symptom | Small random pen-to-ink gaps; stroke op itself is fine |
| Do not | FullClear ordinary ink to “sharpen” |

## Done when

- A device log names the residual steal on that page
- Fix (if the steal is software) does not reintroduce camera FullClear on the pointer stack
- Human confirms the small random gaps are gone or only e-ink
