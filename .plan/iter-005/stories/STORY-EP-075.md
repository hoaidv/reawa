---
id: STORY-EP-075
title: Nested enclose capture and empty-child flatten
kind: implement
parent_srs: [SRS-EP-75, SRS-EP-10]
parent_req: [REQ-05]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-074]
acceptance_criteria:
  - "Given a non-empty Smart Group inside an enclose (Creation A or Enclose CTA), When the create commits, Then that group is a nested child (0 smartgroup_in_selection refuse)."
  - "Given an empty Smart Group (boundary only) inside an enclose, When the create commits, Then the wrapper is gone and its boundary ink is content of the parent."
  - "Given a letter recognized as an empty ink-box plus surrounding free ink, When Creation A enclose captures the cluster, Then the letter ink moves with the new box."
  - "Given paste of an empty ink-box into another ink-box, When paste commits, Then the copy is flattened to content ink."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-075 — Nested enclose capture and empty-child flatten

TRACK-005. [CHL-0032](../challenges/CHL-0032-nested-ink-box.md) Rule 1 + capture.
[SRS-EP-75](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-75-nested-membership).
**Human-verified on device 2026-09-05.**

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-074](./STORY-EP-074.md) |
