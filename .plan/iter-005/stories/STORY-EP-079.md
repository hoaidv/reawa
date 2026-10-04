---
id: STORY-EP-079
title: Migrate point-query callers to geometry index
kind: implement
parent_srs: [SRS-EP-79, SRS-EP-77, SRS-EP-11, SRS-EP-21]
parent_req: [REQ-06]
status: cancelled
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-078, STORY-EP-074]
acceptance_criteria:
  - "Given tap / move-claim / select / finger-on-box / paste-tap, When the press is in world space, Then the winner is the [SRS-EP-79](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries) point query (deepest clipped SmartGroup, later siblings first) — not a new rootChildren walk."
  - "Given nested tap fixtures from [STORY-EP-074](./STORY-EP-074.md), When migrated, Then hit ids agree 100% (child wins; overflow past ancestor clip is not a hit; LOD unchanged)."
  - "Given a connector stroke, When the pen is on the path, Then the connector can be claimed (AABB cull then on-path exact); a press in the path AABB that is off the stroke does not select it ([BR-C11](../../../.docs/modules/epaper/features/connector-ink/srs-product.md))."
  - "Given paste-parent [BR-C13](../../../.docs/modules/epaper/features/clipboard/srs-product.md), When the tap half runs, Then it uses the point query; the 20% ancestor walk stays O(depth) and is not a range scan."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-079 — Migrate point-query callers to geometry index

> **Cancelled 2026-10-04.** This story implemented [ADR-0040](../../../.docs/adr/ADR-0040-logarithmic-hit-test.md) on the archived tree (`epaper_old/`). ADR-0040 is superseded by [ADR-0041](../../../.docs/adr/ADR-0041-document-forest.md), and its parent SRS-EP-78 and SRS-EP-79 by [SRS-EP-81](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality) and [SRS-EP-80](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries). The per-container R-tree is built in [STORY-EP-086](../../iter-006/stories/STORY-EP-086.md) on [TRACK-009](../../tracks/TRACK-009-document-forest.md). The story file is kept for history.

Callers of **point** hit. Index owned by [STORY-EP-078](./STORY-EP-078.md). Product rules
[SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)
(children before ancestors) and [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation)
unchanged.

**Queued.** Nested tap is **done**. Still waits on the index. Do not start while field-latency
[STORY-EP-070](./STORY-EP-070.md)…[STORY-EP-072](./STORY-EP-072.md) are the Lock cursor
unless the human picks this wave.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-078](./STORY-EP-078.md) Spatial R-tree and named geometry queries · [STORY-EP-074](./STORY-EP-074.md) Nested ink-box RenderingContext and tap-select |

## Call sites (must go through the point query)

| Site | Role |
|---|---|
| `nested_inkbox.hpp` `hitTapSmartGroup` | Nested tap |
| `session_doc_context.hpp` `hitMoveTarget` / `hitSelectTarget` / `hitLocalSmartGroup` / `fingerHitsBox` | Tool ports |
| `input_hub.cpp` `tapSelect` | Tap select |
| `move_operation.hpp` | Move claim |
| `paste_action.hpp` | Paste tap (BR-C13 first half) |

A new linear `for (rootChildren)` geometry walk is a defect.

## Done when

- Named point query is the only tap/move/select geometry walk
- Nested tap fixtures 100%
- Clipboard 20% ancestor overlap is still O(depth) after the point query
