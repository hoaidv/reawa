---
id: STORY-IN-016
title: "Draw-into membership for existing Smart Groups"
kind: implement
parent_srs: [SRS-IN-15]
parent_req: [REQ-04]
status: draft
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-IN-012, STORY-IN-014, STORY-IN-010]
acceptance_criteria:
  - "Given a new Pen/ink stroke with ≥80% samples inside one SmartGroup world bounds, When stroke_end runs, Then the ink reparents as role content with its own layoutOffset UV seeded; existing content layoutOffsets unchanged."
  - "Given several qualifying SmartGroups (incl. nested), When membership runs, Then the later sibling (highest paint order) wins; no dual parent."
  - "Given no qualifying group, When stroke_end runs, Then ink stays under its ordinary parent."
  - "Given membership, When applied, Then SmartGroup bounds are not expanded; one undo restores the prior snapshot."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-016 — Draw-into membership

Implements [SRS-IN-15](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-15-draw-into-membership).
**No design** (logic-only; optional beat in ink-box package).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | IN-012, IN-014, IN-010 |

## Done when

- AC green `@SRS-IN-15`
