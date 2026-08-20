---
id: STORY-IN-035
title: Persist and restore pen-button map (not the editor)
kind: implement
parent_srs: [SRS-IN-23, SRS-IN-25]
parent_req: [REQ-05]
status: cancelled
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-056]
acceptance_criteria:
  - "Given the creator binds a map on the tablet, When Infini is connected, Then Infini persists that map (not in the document) and sends 0 document messages for that persist."
  - "Given a later session with a persisted map and matching 1- or 2-button capability, When hello completes, Then the tablet next gesture uses that map (p95 <=300 ms after restore) and in-flight gestures are unchanged."
  - "Given a 0-button pen, When Infini restores, Then 0 fake button bindings are applied."
  - "Given Infini has no session, When the tablet edits the map, Then the live device map still applies (persist waits; 0 lost local binds)."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ".plan/iter-005/design/pen-button-map/ui-spec.md"
scenes:
  - ".plan/iter-005/design/pen-button-map/pen-button-map-entry.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-0.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-2.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-offline.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-chip-temp-erase.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-chip-drag.html"
hifi: ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
wireframe: ""
---

# STORY-IN-035 — Persist and restore pen-button map (not the editor)

**Parked 2026-08-20 — do not implement.** Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore is **retired**. Device Settings persist on Epaper ([REQ-20](../../../.docs/modules/epaper/prd.md#device-settings)) via [STORY-EP-057](./STORY-EP-057.md). Keep this id.

Was: Infini persist/restore only — no desktop editor. Bound: [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md), [SRS-IN-25](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md). [ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) persist split is pending supersede.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-056 |
