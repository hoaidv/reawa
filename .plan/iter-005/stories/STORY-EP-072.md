---
id: STORY-EP-072
title: Camera-change stress probe on a full handwriting page
kind: implement
parent_srs: [SRS-EP-03, SRS-EP-24]
parent_req: [REQ-02, REQ-10]
status: ready
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given two-finger pan/pinch on a full page of handwriting plus ink-boxes, When the camera moves, Then /tmp/epaper-raster.log (or an extended camera-job log) records blit vs LatestJob deliver vs settle submit, including job id, snap epoch, cancelled/dropped/held/warped/swapped, render_ms, and whether m_cameraNeedsSharp is still owed."
  - "Given continuous pan faster than one vector (~700 ms on a dense page), When in-flight is allowed to finish, Then the log shows mid-gesture delivers (possibly warped) and a final sharp at finger-up — not cam=none skip with no job."
  - "Given excessive zoom-in, When the gesture ends, Then the log shows a vector swap for that camera (blit=0 sharp=1) so white strips / blocky zoom cannot be silent misses."
  - "Given the stress page, When the human tails the log, Then a short device recipe lives in .docs/memory/camera-pan-zoom-rasterize.md (how to fill the page, which lines mean success)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-072 — Camera-change stress probe on a full handwriting page

Field 2026-08-31: blit + LatestJob camera pipeline is **better** (smooth pan/zoom, sharpen no
longer random on a light page). Need **stronger instrumentation under stress**: a **full page** of
handwriting and ink-boxes, so job cancel/keep, warp, settle-submit, and strip-fill are visible
in the log — not only felt.

Canonical: [SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md#srs-ep-03-map-before-refresh-coalesce-and-stroke-fidelity);
[SRS-EP-24](../../../.docs/modules/epaper/features/region-sync/srs-logic.md#srs-ep-24-two-finger-viewport)
two-finger viewport;
memory [camera-pan-zoom-rasterize.md](../../../.docs/memory/camera-pan-zoom-rasterize.md).

This story is **instrument + stress recipe**. Pipeline architecture already landed; do not put
vector FullClear back on the pointer stack.

Human is Quality Assurance Engineer on device. No design package. No behavior-driven ceremony.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Stress page

| | |
|---|---|
| Content | Full panel of handwriting **and** ink-boxes (harder than the 4-sentence page) |
| Gestures | Pan, pinch zoom-in (including excessive), pinch zoom-out, panzoom, settle |
| Success lines | `blit=1` during nav; later `blit=0 sharp=1` (cam=none means exact match, not skip) |

## Done when

- Log distinguishes preview blit, in-flight keep, warp, drop-on-ink-epoch, settle swap
- Memory doc has the stress recipe
- Human can reproduce on RM2 with `tail -f /tmp/epaper-raster.log`
