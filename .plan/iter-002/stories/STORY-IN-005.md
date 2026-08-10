---
id: STORY-IN-005
title: "Verify gesture frame budget on 60Hz display"
kind: implement
parent_srs: [SRS-IN-03]
parent_req: [REQ-01]
status: draft
priority: P1
iter: iter-002
estimate: 2
owner: dev
depends_on: [STORY-IN-004]
acceptance_criteria:
  - "Given a 60 Hz display, When the user pans continuously for ≥5 s, Then perceived dropped frames are ≤2/s (manual or rAF counter evidence recorded in story notes)."
  - "Given pinch or modifier+wheel zoom for ≥5 s, When measuring the same budget, Then the target holds and circle aspect remains circular."
  - "Given ADR-0008 risk, When the spike fails the budget, Then a challenge is filed (do not silently switch shells)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-005 — Verify gesture frame budget on 60Hz display

Implements quality scenario [SRS-IN-03](../../../.docs/modules/infini/features/infinity-canvas/srs-quality.md).
Depends on [STORY-IN-004](./STORY-IN-004.md).
