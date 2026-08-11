---
id: STORY-IN-011
title: Drawing-region marker + coalesced viewport publish
kind: implement
parent_srs: [SRS-IN-07]
parent_req: [REQ-03]
status: done
priority: P1
iter: iter-002
estimate: 3
owner: dev
depends_on: [STORY-IN-009]
# follow-up 2026-08-11: wired Infini→RM region_refresh + max-fit frame + orientation toggle
acceptance_criteria:
  - "Given Infini is idle (not panning/zooming), When the canvas is shown, Then the tablet drawing-region marker is not visible."
  - "Given the user starts pan or zoom, When the gesture is active, Then the tablet drawing-region marker is visible and tracks the current drawingRegion AABB."
  - "Given pan/zoom ends, When the gesture settles, Then the marker hides (after optional short fade) without leaving a persistent chrome frame."
  - "Given the user pans or zooms, When the viewport changes, Then Infini emits viewport messages (type, translate, scale, drawingRegion AABB, monotonic seq) toward Epaper so the tablet-sync region follows the canvas under the marker."
  - "Given rapid pan/zoom frames, When publishing, Then viewport emits are coalesced (latest wins) so Epaper is not flooded; map fields remain current for the next pen sample budget in SRS-IN-08."
  - "Given the same world ink and drawingRegion, When comparing Infini screen stroke thickness to Epaper region stroke thickness, Then apparent size matches within the architect-defined tolerance (world-unit width + shared scale rule)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-011 — Drawing-region marker + coalesced viewport publish

Implements live Infini half of [ADR-0009](../../../.docs/adr/ADR-0009-shared-document-viewport.md)
viewport channel wiring into the canvas. Parent: [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-009; architect thicken done (READY-WITH-CONCERNS) |
| Design | **none** — REQ-03 Needs design: no |

## Architect notes

- Tablet CSS frame → `drawingRegion` (not full-window AABB) — [ADR-0012](../../../.docs/adr/ADR-0012-world-stroke-viewport-parity.md)
- Publish ≤30 Hz + settle flush; marker only while gesturing

## Done when

- BDD green; CanvasStage / `TabletSession` wired; pair with EP-002 for E2E.
