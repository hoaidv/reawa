---
id: STORY-EP-056
title: Revise pen-button map as Epaper on-device editor
kind: design
parent_srs: [SRS-EP-52, SRS-EP-53, SRS-EP-42]
parent_req: [REQ-18, REQ-20]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: designer
depends_on: [STORY-IN-034]
acceptance_criteria:
  - "Given REQ-18 on-device editor journeys, When the package ships, Then one scene HTML exists per listed journey and ui-spec-gate passes."
  - "Given Click and Hold-move lists, When shown, Then Click is only current-tool <-> Freeform Select, current-tool <-> Eraser, and Off; Hold-move is only Temporary eraser, Drag node under tip, and Off."
  - "Given the package, When shown, Then platform is epaper-device (1-bit, no hover) and 0 Infini desktop settings chrome remains."
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

# STORY-EP-056 — Revise pen-button map as Epaper on-device editor

Follow-on to [STORY-IN-034](./STORY-IN-034.md) (`done` as Infini desktop — **not** the product). Same package `pen-button-map/`. Parents [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) and [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings). Human 2026-08-20: editor is **epaper-device**, not Infini. [CHL-0025](../challenges/CHL-0025-pen-map-settings-page.md) **adopted**: one Settings page (master-detail); catalogues inline. GAP-01 leading tile adopted.

**Do not** bury viewport-follow or hand-touch scenes here. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore is **retired**. Persist is [STORY-EP-057](./STORY-EP-057.md).

`parent_srs`: [SRS-EP-52](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) On-device pen-button map editor · [SRS-EP-53](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author) On-device pen-button map authoring. Scene graph: `epaper/tool-modes/srs-ui-multi-scene.md`. Bound 2026-08-20 ([ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md)).

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | STORY-IN-034 (historical Infini paint; this story replaces it) |
