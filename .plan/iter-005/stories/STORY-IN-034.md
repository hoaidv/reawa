---
id: STORY-IN-034
title: Design Infini pen-button map settings
kind: design
parent_srs: [SRS-IN-24, SRS-IN-23]
parent_req: [REQ-05]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given 0, 1, and 2 button capability, When the package ships, Then each layout has a scene and ui-spec-gate passes."
  - "Given Click vs Hold-move catalogues, When shown, Then click list is discrete-only and hold-move list is temporary-tool-only (D9)."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ".plan/iter-005/design/pen-button-map/ui-spec.md"
scenes:
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-0.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-2.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-slot-click.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-slot-hold.html"
hifi: ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
wireframe: ""
---

# STORY-IN-034 — Design Infini pen-button map settings

TRACK-005 Wave 1-B. **Historical.** Painted as Infini desktop settings ([UI-IN-03](../../../.docs/design/index.md)). Human 2026-08-20: the package is **Epaper**, not Infini. Do **not** delete this id.

Current product: [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) on-device editor. Revision story: [STORY-EP-056](./STORY-EP-056.md). Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) is persist/restore only.

Do not treat this story’s scenes as the shipping user interface.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |
