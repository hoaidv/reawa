---
id: STORY-IN-034
title: Design Infini pen-button map settings
kind: design
parent_srs: [SRS-IN-05]
parent_req: [REQ-05]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given 0, 1, and 2 button capability, When the package ships, Then each layout has a scene and ui-spec-gate passes."
  - "Given Click vs Hold-move catalogues, When shown, Then click list is discrete-only and hold-move list is temporary-tool-only (D9)."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-034 — Design Infini pen-button map settings

TRACK-005. Parent [REQ-05]. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) · epaper [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) D9 catalogues.

Package `pen-button-map/`. Desktop settings: 0/1/2 button layouts; Click and Hold-move closed lists; invalid/stale map; offline then publish.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
