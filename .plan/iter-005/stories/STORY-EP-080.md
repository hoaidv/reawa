---
id: STORY-EP-080
title: Migrate range 80-percent callers to geometry index
kind: implement
parent_srs: [SRS-EP-79, SRS-EP-11, SRS-EP-10, SRS-EP-75, SRS-EP-58]
parent_req: [REQ-06]
status: draft
priority: P0
iter: iter-005
estimate: 8
owner: dev
depends_on: [STORY-EP-078, STORY-EP-076]
acceptance_criteria:
  - "Given sel_rect or sel_freeform, When the gesture commits, Then candidates come from AABB-overlap cull on the index then exact ≥80% on k; marquee/freeform stay **top-level**; Ink and Connector use path-sample 80%; grazing AABB intersect does not select."
  - "Given enclose capture or draw-into membership, When they run, Then they use the named range queries; draw-into includes **nested** SmartGroups (do not copy today’s top-level-only `membership.hpp` walk); exact 80% is still sample-count / natural-area / polyline-length — never AABB-only."
  - "Given move-commit reparent, When ≥80% of the moving node’s natural world rect sits in a container, Then the winner is the highest-paint named query (exclude self and descendants); else document root. Resize does not reparent."
  - "Given object erase, When candidates are collected, Then the index supplies AABB overlap cull only; the exact 80% table stays [SRS-EP-58](../../../.docs/modules/epaper/features/erase/srs-logic.md#srs-ep-58-object) (arc length / boundary area) and stays off the pointer-move path."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-080 — Migrate range 80-percent callers to geometry index

Callers of **rect / polygon / highest-paint-container / membership / enclose** 80% tests,
plus object-erase **cull only**. Index owned by [STORY-EP-078](./STORY-EP-078.md).

Product 80% bars unchanged. Approximating 80% from AABB overlap is a challenge, not this story.

**Queued.** Move-reparent [STORY-EP-076](./STORY-EP-076.md) is **done**. Still waits on the index.
Do not start while field-latency is the Lock cursor unless the human picks this wave.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-078](./STORY-EP-078.md) Spatial R-tree and named geometry queries · [STORY-EP-076](./STORY-EP-076.md) Reparent nested ink-box at end of move |

## Call sites (must go through named range queries)

| Site | Role |
|---|---|
| `surround_create.hpp` `selectByRect` / `selectByFreeform` | Marquee / freeform |
| `marquee_operation.hpp` / `lasso_operation.hpp` | Gesture callers |
| `nested_inkbox.hpp` `chooseMoveParentId` + `transform_gesture.hpp` | Move-commit reparent |
| `membership.hpp` + `recognizer_dispatch.hpp` | Draw-into (nested groups) |
| `recognize_enclose.hpp` | Enclose capture |
| `erase_object.hpp` | AABB cull only |

**Out:** clipboard 20% ancestor overlap (point story); area-erase clip; endpoint-ink connector scan;
Infini desktop pick.

## Done when

- No leftover linear geometry walk on these callers
- Nested / enclose / marquee fixtures 100%
- Object-erase exact table unchanged; overlay 80% still off the pointer path
