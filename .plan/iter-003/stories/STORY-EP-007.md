---
id: STORY-EP-007
title: "Selection mode must not draw ink"
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — device-side behaviour re-specified under epaper REQ-04…REQ-07; code discarded by the restore. Not scheduled; SM re-slices."
priority: P0
iter: iter-003
estimate: 1
owner: dev
depends_on: [STORY-EP-006]
acceptance_criteria:
  - "Given Selection tool armed, When pen moves without active pick gesture, Then no stroke begins on tablet."
  - "Given miss pick on press, When pen moves, Then still no ink."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-007 — Selection mode must not draw ink

Guard move/release in `TabletCanvasItem::ingestPoint` when `toolMode == selection`.
