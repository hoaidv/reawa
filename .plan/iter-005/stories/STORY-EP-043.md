---
id: STORY-EP-043
title: Design copy/cut/paste on selection overlay
kind: design
parent_srs: [SRS-EP-32, SRS-EP-31]
parent_req: [REQ-12]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-12 UI states, When the package ships, Then one scene per state and ui-spec-gate passes."
  - "Given empty clipboard, When paste is shown, Then the spec marks it a no-op."
design_package: ".plan/iter-005/design/clipboard-chrome/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-043 — Design copy/cut/paste on selection overlay

TRACK-005. Parent [REQ-12]. [REQ-12](../../../.docs/modules/epaper/prd.md#clipboard)

Package `clipboard-chrome/`. States: copy/cut/paste on selection; empty clipboard; paste offset visible; undo after cut+paste. No OS clipboard.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
