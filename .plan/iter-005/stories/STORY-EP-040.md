---
id: STORY-EP-040
title: Design erase: nib feedback and selection-erase CTA
kind: design
parent_srs: [SRS-EP-29, SRS-EP-27, SRS-EP-28]
parent_req: [REQ-11]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-11 UI states, When the package ships, Then one scene per state and ui-spec-gate passes."
  - "Given empty selection, When Erase is shown, Then the spec marks it a no-op (no hidden delete-all)."
  - "Given no eraser nib, When Pen is used, Then the spec does not imply Path A fires."
design_package: ".plan/iter-005/design/erase-chrome/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-040 — Design erase: nib feedback and selection-erase CTA

TRACK-005. Parent [REQ-11]. [REQ-11](../../../.docs/modules/epaper/prd.md#erase)

Package `erase-chrome/`. States: nib in progress; selection-erase CTA; empty selection no-op; undo after erase; missing nib.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
