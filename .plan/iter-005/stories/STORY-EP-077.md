---
id: STORY-EP-077
title: Clip nested ink-box content to natural world AABB
kind: implement
parent_srs: [SRS-EP-76, SRS-EP-77]
parent_req: [REQ-06]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-074]
acceptance_criteria:
  - "Given a nested ink-box whose natural AABB lies fully outside its parent’s natural AABB, When the document paints, Then the child’s content is not emitted."
  - "Given that overflow child, When the creator taps the child’s AABB, Then the child is not selected."
  - "Given a nested child whose AABB overlaps the parent AABB, When the creator taps the overlap, Then the child is still selected."
  - "Clip is the natural world AABB (hull of local bounds after outcome), not even-odd of boundary ink."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-077 — Clip nested ink-box content to natural world AABB

TRACK-005. [CHL-0032](../challenges/CHL-0032-nested-ink-box.md) AABB-clip amend.
[SRS-EP-76](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-76-nested-render)
content clip / [SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)
overflow not hittable. [ADR-0039](../../../.docs/adr/ADR-0039-nested-ink-box-rendering.md) §6.

Fixes stale dirty-rect paint when `fixedInk` then `withBounds` leaves a child outside the parent AABB.
Does **not** walk descendant handwriting for a visual AABB. **Human-verified on device 2026-09-05.**

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-074](./STORY-EP-074.md) |
