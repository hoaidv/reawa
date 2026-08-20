---
id: STORY-EP-052
title: Barrel click vs hold-move dispatch from catalogue
kind: implement
parent_srs: [SRS-EP-41, SRS-EP-43]
parent_req: [REQ-18]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-056]
acceptance_criteria:
  - "Given a 1-button pen and defaults, When the creator clicks (no move), Then exclusive tool toggles current primary <-> sel_freeform p95 <=300 ms and 0 hold-move runs."
  - "Given the same pen and defaults, When hold-move, Then temporary erase runs until release and 0 click fires on release."
  - "Given a 2-button pen and defaults, When B2 hold-moves, Then temporary erase runs until release."
  - "Given Hold-move rebound to drag-under-tip, When starting on a hittable node, Then that node moves (REQ-06); when starting on empty, Then 0 move and 0 lasso."
  - "Given a 0-button pen, When drawing, Then 0 button gestures fire and REQ-03 still works."
  - "Given a 20-gesture fixture mixing clicks and holds, When executed, Then 0 events fire both click and hold-move."
  - "Given a rebind mid-session, When a gesture is in flight, Then the in-flight binding does not change."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ".plan/iter-005/design/pen-button-map/ui-spec.md"
scenes:
  - ".plan/iter-005/design/pen-button-map/pen-button-map-entry.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-0.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-2.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-offline.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-slot-click.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-slot-hold.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-chip-temp-erase.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-chip-drag.html"
hifi: ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
wireframe: ""
---

# STORY-EP-052 — Barrel click vs hold-move dispatch from catalogue

TRACK-005. Parent [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons). Catalogues and defaults as of 2026-08-20: Click current ↔ Freeform Select / current ↔ Eraser / Off; Hold-move Temporary eraser / Drag-under-tip / Off. Default 1-button Hold-move is **Temporary eraser** (not temporary freeform).

Depends on [STORY-EP-056](./STORY-EP-056.md) (epaper-device package). Do not implement against the historical Infini desktop paint of [STORY-IN-034](./STORY-IN-034.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-056 |
