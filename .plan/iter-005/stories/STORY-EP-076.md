---
id: STORY-EP-076
title: Reparent nested ink-box at end of move
kind: implement
parent_srs: [SRS-EP-77]
parent_req: [REQ-06]
status: done
priority: P1
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-074]
acceptance_criteria:
  - "Given a nested child moved so ≥80% of its natural area lies inside a different box, When the move commits, Then that highest-paint box is the new parent."
  - "Given a nested child moved so <80% of its natural area lies inside every container, When the move commits, Then its parent is the document root."
  - "Given a resize (not move), When it commits, Then the parent is unchanged."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-076 — Reparent nested ink-box at end of move

TRACK-005. [CHL-0032](../challenges/CHL-0032-nested-ink-box.md) Rule 5.
[SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent).
**Human-verified on device 2026-09-05.**

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-074](./STORY-EP-074.md) |
