---
id: STORY-EP-016
title: "On-device enclose recognition"
kind: implement
parent_srs: [SRS-EP-10, SRS-EP-14]
parent_req: [REQ-05]
status: draft
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-014, STORY-EP-015]
acceptance_criteria:
  - "Given Ink-box armed at pen-down, When pen-up enclose guards pass, Then create_smart_group commits locally with 0 desktop messages, the enclose stroke is role boundary, captured ink is role content with layoutOffset UV, bounds equal the fitted AABB, and the box is visible p95 ≤500 ms after pen-up."
  - "Given Pen armed, or a stroke too small / empty, When pen-up runs, Then 0 boxes are created and the stroke stays ordinary ink (0 error banner)."
  - "Given Ink-box armed at pen-down, When the tool switches before pen-up, Then the stroke is still evaluated as an enclose."
  - "Given 10 consecutive successful encloses, When they complete, Then 10 / 10 boxes exist with 0 desync and 0 lost boxes (CHL-0007 regression)."
  - "Given shared fixtures enclose/, When device and desktop evaluate the same stroke, Then guard verdict and fitted bounds agree 100%."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-016 — On-device enclose recognition

Implements enclose rules in [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md)
and create-quality bars in [SRS-EP-14](../../../.docs/modules/epaper/features/ink-box/srs-quality.md).
BDD: [enclose-recognition.feature](../../../.docs/modules/epaper/features/ink-box/bdd/enclose-recognition.feature).

**No peer round trip inside the gesture.** Recognition is not an op; only `create_smart_group` publishes.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-014, EP-015 |

## Done when

- `@SRS-EP-10` enclose scenarios green; `enclose/` fixtures agree
