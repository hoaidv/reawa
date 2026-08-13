---
id: STORY-EP-017
title: "On-device draw-into membership"
kind: implement
parent_srs: [SRS-EP-10, SRS-EP-14]
parent_req: [REQ-05]
status: draft
priority: P1
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-EP-016]
acceptance_criteria:
  - "Given a new Pen stroke with ≥80% of samples inside one SmartGroup bounds, When pen-up membership runs, Then the ink is reparented as role content with layoutOffset UV seeded, p95 ≤300 ms, and 0 existing content inks move."
  - "Given several overlapping candidate groups, When membership runs, Then the highest paint/z order (later sibling) wins and 0 dual-parented ink exists."
  - "Given an enclose stroke, When pen-up runs, Then membership does not run on that stroke."
  - "Given membership, When bounds are observed, Then SmartGroup bounds are not expanded by the new ink."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-017 — On-device draw-into membership

Implements draw-into rules in [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md).
BDD: [draw-into-membership.feature](../../../.docs/modules/epaper/features/ink-box/bdd/draw-into-membership.feature).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-016 |

## Done when

- `@SRS-EP-10` draw-into scenarios green
