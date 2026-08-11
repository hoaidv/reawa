---
id: STORY-IN-014
title: "Snapshot undo ring for vector document"
kind: implement
parent_srs: [SRS-IN-12]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-IN-012]
acceptance_criteria:
  - "Given a structural op (create/reparent/remove/set_smart_transform/set_ink_scale_mode), When applied, Then a snapshotString entry is pushed before the op (ring depth 20)."
  - "Given at least one ring entry, When undo runs, Then the tree equals the pre-op snapshot exactly."
  - "Given viewport pan/zoom or tool/selection changes, When they occur, Then they do not push undo entries."
  - "Given ring overflow, When a 21st snapshot would push, Then the oldest drops and undo past empty is a no-op."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-014 — Snapshot undo ring

**In review.** `UndoRing` in `infini/src/document/UndoRing.ts`. BDD: `bdd/undo-ring.feature`.
Tests: `infini/tests/undo-ring.test.ts`.
