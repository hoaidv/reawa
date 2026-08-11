---
id: STORY-IN-004
title: "Wire pan zoom pinch gesture inputs"
kind: implement
parent_srs: [SRS-IN-02]
parent_req: [REQ-01]
status: in-review
priority: P1
iter: iter-002
estimate: 5
owner: dev
depends_on: [STORY-IN-001, STORY-IN-003]
acceptance_criteria:
  - "Given Infini is focused, When the user pans via trackpad, mouse drag, wheel, or modifier+wheel, Then translate updates and the canvas moves without teleport jumps."
  - "Given Infini is focused, When the user pinches or uses modifier+wheel zoom, Then uniform scale changes about the gesture focus (or documented fallback from design)."
  - "Given canvas.gesturing, When a gesture is in progress, Then no modal chrome blocks the canvas."
  - "Given window resize (canvas.resized), When size changes, Then the world anchor matches the design decision (center or top-left)."
design_package: ".plan/iter-002/design/infinity-canvas/"
ui_spec: ".plan/iter-002/design/infinity-canvas/ui-spec.md"
scenes:
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-empty.html"
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-populated.html"
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-gesturing.html"
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-resized.html"
hifi: ".plan/iter-002/design/infinity-canvas/infinity-canvas-populated.html"
wireframe: ""
---

# STORY-IN-004 — Wire pan zoom pinch gesture inputs

Implements gesture map in [SRS-IN-02](../../../.docs/modules/infini/features/infinity-canvas/srs-ui.md).
Depends on [STORY-IN-001](./STORY-IN-001.md) + [STORY-IN-003](./STORY-IN-003.md).
