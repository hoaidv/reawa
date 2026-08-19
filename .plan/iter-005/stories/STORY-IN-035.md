---
id: STORY-IN-035
title: Persist and publish pen-button map to tablet
kind: implement
parent_srs: [SRS-IN-23, SRS-IN-25]
parent_req: [REQ-05]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-IN-034]
acceptance_criteria:
  - "Given 1- or 2-button capability, When the creator assigns each slot and saves, Then the next tablet gesture uses that map (p95 <=300 ms after publish) and in-flight gestures are unchanged."
  - "Given a 0-button pen, When settings are shown, Then barrel slots are absent or disabled (0 fake bindings)."
  - "Given a live session, When the map is published, Then Infini sends 0 document messages for that publish (settings channel)."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ".plan/iter-005/design/pen-button-map/ui-spec.md"
scenes:
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-0.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-2.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-slot-click.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-slot-hold.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-invalid-stale.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-offline-then-publish.html"
hifi: ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
wireframe: ""
---

# STORY-IN-035 — Persist and publish pen-button map to tablet

TRACK-005. Parent [REQ-05]. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map)



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-034 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
