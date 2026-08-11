---
id: STORY-IN-012
title: "Tree-backed live ink ingestion (append_ink path)"
kind: implement
parent_srs: [SRS-IN-04]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-IN-007, STORY-IN-009]
acceptance_criteria:
  - "Given a live pen stroke on Infini (or append_ink from Epaper), When the stroke commits, Then samples land as Ink under the VectorDocument tree (not only flat WorldLayer primitives)."
  - "Given tree-backed ink, When syncFromVectorDoc / flattenDrawables runs, Then the canvas paints that ink via the existing WorldLayer path."
  - "Given rebuildWithRmInk-style refresh, When tree ink exists, Then the tree remains the SoT for those strokes (no silent drop to WorldLayer-only)."
  - "Given this story done, When Smart Group stories run, Then recognize_enclose / membership can see Ink nodes in the tree."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-012 — Tree-backed live ink ingestion

**In review.** `commitLiveStrokeToTree` + CanvasStage `stroke_end` → tree; paint via `syncFromVectorDoc`.
BDD: `bdd/tree-backed-ink.feature`. Tests: `infini/tests/tree-backed-ink.test.ts`.
