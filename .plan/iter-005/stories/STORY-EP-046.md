---
id: STORY-EP-046
title: Apply per-end connector styles from toolbar
kind: implement
parent_srs: [SRS-EP-34, SRS-EP-37]
parent_req: [REQ-13]
status: blocked
priority: P1
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-045]
acceptance_criteria:
  - "Given a selected connector, When the creator picks an end style, Then that end shows it p95 <=300 ms and the other end is unchanged; one undo reverts."
  - "Given a connector with end styles, When a bound box is dragged, Then styles stay on the correct ends and REQ-09 warp bar holds (0 px jump)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-046 — Apply per-end connector styles from toolbar

**Frozen 2026-09-04 (Product Manager).** Path A is not Epaper this campaign. Depends on frozen [STORY-EP-045](./STORY-EP-045.md).



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-045 |

Stories stay **draft** until `/architect` binds dedicated SRS (current parent_srs is the nearest existing section).
