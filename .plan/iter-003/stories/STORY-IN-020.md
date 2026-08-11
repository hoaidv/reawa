---
id: STORY-IN-020
title: "Push doc_snapshot after Smart Group mutations"
kind: implement
parent_srs: [SRS-IN-11]
parent_req: [REQ-04]
status: in-review
priority: P0
iter: iter-003
estimate: 2
owner: dev
depends_on: [STORY-IN-019]
acceptance_criteria:
  - "Given desktop move/resize commits set_smart_transform, When gesture completes, Then tablet receives fresh doc_snapshot + pickables."
  - "Given enclose creates Smart Group from tablet, When stroke_end succeeds, Then tablet receives doc_snapshot (not viewport-only)."
  - "Given surround create on desktop, When create succeeds, Then tablet receives doc_snapshot."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-020 — Push doc_snapshot after Smart Group mutations

Implemented `pushDocSnapshotToRm()` in `CanvasStage.tsx` after transform commit, surround create, enclose create, tool_intent.
