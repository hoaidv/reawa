---
id: STORY-IN-003
title: "Canvas transform and primitive figures"
kind: implement
parent_srs: [SRS-IN-01]
parent_req: [REQ-01]
status: draft
priority: P1
iter: iter-002
estimate: 5
owner: dev
depends_on: [STORY-IN-001, STORY-IN-002]
acceptance_criteria:
  - "Given world-space primitives (line, rect, ellipse/circle, path), When rendered, Then screen positions match screen = (world + translate) * scale with uniform scale > 0."
  - "Given a circle in world space, When the user pans and zooms (programmatic transform OK in tests), Then the circle stays circular (aspect preserved)."
  - "Given canvas.empty vs populated, When vectors are absent or present, Then both states render without crash."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-003 — Canvas transform and primitive figures

Implements [SRS-IN-01](../../../.docs/modules/infini/features/infinity-canvas/srs-logic.md) transform model.
Depends on [STORY-IN-001](./STORY-IN-001.md) + [STORY-IN-002](./STORY-IN-002.md).
