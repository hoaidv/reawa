---
id: STORY-IN-031
title: Remove Infini desktop editing ToolStrip
kind: implement
parent_srs: [SRS-IN-14]
parent_req: [REQ-04]
status: in-review
priority: P0
iter: iter-004
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given the Infini desktop window this campaign, When the creator looks for an ink-box or selection tool, Then 0 ToolStrip, selection overlay, or transform handles are offered (REQ-04)."
  - "Given a Smart Group in the mirror, When the window is shown, Then pan/zoom and open/save chrome still work and the group still paints."
  - "Given leftover ToolStrip.tsx / SelectionOverlay wiring, When this story is done, Then those affordances are unmounted (no data-region=ToolStrip)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-031 — Remove Infini desktop editing ToolStrip

Removes leftover Infini authoring chrome so the desktop matches deprecated
[REQ-04](../../.docs/modules/infini/prd.md#smart-group) / [SRS-IN-14](../../.docs/modules/infini/features/vector-document/srs-ui.md).
**Not** a design story — PM waived `/designer` (hide, do not paint). **∥ [STORY-EP-028](./STORY-EP-028.md)** (`infini/` vs `epaper/`).

BDD: `.docs/modules/infini/features/vector-document/bdd/hide-editing-toolbar.feature`.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | none |
| Parallel | EP-028 |
