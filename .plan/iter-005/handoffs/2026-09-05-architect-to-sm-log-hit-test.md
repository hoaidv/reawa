---
from: architect
to: sm
date: 2026-09-05
iter: iter-005
---

# Hand-off: Architect → Scrum Master

## Context

Human asked for logarithmic hit-test (tap, freeform, rect, highest-z container ≥80% of a rectangle, …). Bound as a **device document-geometry query API** over **one R-tree**. Not a new journey. `needs_design: no`. Product hit rules unchanged ([SRS-EP-75](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-75-nested-membership) / [SRS-EP-76](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-76-nested-render) / [SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)). Do **not** reopen [CHL-0011](../challenges/CHL-0011-nested-smartgroup-enclose.md). Nested implement [STORY-EP-074](../stories/STORY-EP-074.md)…[STORY-EP-077](../stories/STORY-EP-077.md) are in-review — index stories **depend_on** nested tap at least.

| ID | Title |
|---|---|
| [SRS-EP-78](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-78-log-hit-test) | Logarithmic document-geometry query |
| [SRS-EP-79](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries) | Document geometry queries (spatial index) |
| [ADR-0040](../../../.docs/adr/ADR-0040-logarithmic-hit-test.md) | Device logarithmic hit-test spatial index (`proposed`) |

≤100 ms selection feel stays in [SRS-EP-14](../../../.docs/modules/epaper/features/ink-box/srs-quality.md) / [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md). Complexity is **probes vs n** (500-node product fixture **and** 5000-node stress). Ink p95 ≤30 ms still outranks everything.

## Review verdict

**READY-WITH-CONCERNS**

### Strengths

| Finding | Evidence |
|---|---|
| Exact 80% preserved | ADR-0040 Decision 5; approximating 80% from AABB is named as a `CHL-*`, not a silent change |
| One index, family of queries | SRS-EP-79 named table; no second tree |
| Nested z is not AABB-max | Paint-rank among candidates; clipped composed AABB ([ADR-0039](../../../.docs/adr/ADR-0039-nested-ink-box-rendering.md)) |
| Ink path isolated | Commit-only STR rebuild; **0** live-preview index writes |
| Object-erase / paste-20% / Infini paint quadtree scoped | Explicit out-of-API rows |

### Concerns

| Finding | Why it is not blocking |
|---|---|
| Nested EP-074…077 still in-review | Slice `depends_on` nested tap (EP-074) / reparent (EP-076); do not implement against the old top-level walk |
| `membership.hpp` today does **not** recurse into nested SmartGroups | SRS-EP-10 already says “incl. nested”; migrate must not copy the old walk |
| Rebuild-every-op 5 ms / 50 ms rows are implement locks until measured | A miss files `CHL-*` (incremental R*), not a product change |
| Infini `pickSmartGroupAt` stays linear | Device is the writer; desktop index is a later Infini story, not this campaign |
| Probe-count constant `64·⌈log₂(n)⌉+k` | Locks the **O()** class; retune the constant if STR fanout differs — do not drop the linear-fail row |

### Risks

None blocking slice. Pathological k (every AABB overlaps the query) still costs O(k) exact tests — accepted; miss → `CHL-*`, do not loosen 80%.

## Asks

1. Slice **three** implement stories (no design story):

   | Suggested title | Owns | `parent_srs` |
   |---|---|---|
   | Spatial R-tree + named geometry queries | Index, STR rebuild, 500/5000 fixtures, probe counters, API | [SRS-EP-79], [SRS-EP-78] |
   | Migrate point-query callers | Tap / move / select / finger / paste-tap / connector on-stroke | [SRS-EP-79], [SRS-EP-77], [SRS-EP-11], [SRS-EP-21] |
   | Migrate range 80% callers | Marquee, freeform, enclose capture, draw-into, move-reparent; object-erase **cull only** | [SRS-EP-79], [SRS-EP-11], [SRS-EP-10], [SRS-EP-75], [SRS-EP-58] |

2. Do **not** start EP-070 / EP-071 / EP-072 / EP-073. WIP 2 still applies — pick two of the three after nested is unblocked enough, or queue behind EP-070…072 per the Lock cursor.
3. `/qa` BDD: product agreement 100% vs nested/enclose/marquee fixtures **and** a probe-count scenario tagged `@SRS-EP-78`.

## Call-site inventory

Must go through SRS-EP-79 (today’s linear walks):

| Site | Role |
|---|---|
| `epaper/document/nested_inkbox.hpp` `hitTapSmartGroup` | Nested tap |
| `epaper/drawing/tools/contexts/session_doc_context.hpp` `hitMoveTarget` / `hitSelectTarget` / `hitLocalSmartGroup` / `fingerHitsBox` | Tool ports |
| `epaper/drawing/tools/input_hub.cpp` `tapSelect` → `select_operation.hpp` | Tap select |
| `epaper/drawing/tools/operations/move_operation.hpp` | Move claim |
| `epaper/drawing/tools/actions/paste_action.hpp` | Paste tap (BR-C13 first half) |
| `epaper/document/surround_create.hpp` `collectPickable` / `selectByRect` / `selectByFreeform` | Marquee / freeform (include Connector path-sample 80%, [BR-C11](../../../.docs/modules/epaper/features/connector-ink/srs-product.md)) |
| `epaper/drawing/tools/operations/marquee_operation.hpp` / `lasso_operation.hpp` | Gesture callers |
| `epaper/document/nested_inkbox.hpp` `chooseMoveParentId` + `transform_gesture.hpp` | Move-commit reparent |
| `epaper/document/membership.hpp` `qualifyingMembershipGroup` / `smartGroupsInPaintOrder` + `recognizer_dispatch.hpp` | Draw-into (nested SGs) |
| `epaper/document/recognize_enclose.hpp` `walkInkCandidates` / `collectTopLevelSmartGroups` / empty `insideExisting` loop | Enclose capture |
| `epaper/document/erase_object.hpp` `collectObjectEraseSubjects` / overlay job | **AABB cull only**; exact table stays SRS-EP-58 |

Stays **out** (do not invent range queries):

| Site | Why |
|---|---|
| `epaper/drawing/tools/clipboard.hpp` `chooseParent` / `overlaps20` | O(depth) after the point query |
| Area erase clip | Geometric clip, not 80% hit |
| `endpoint_ink.hpp` `collectConnectors` | Small n; optional cull later |
| Infini `SpatialIndex.ts` quadtree / `pickSmartGroupAt` | Paint cull + desktop pick; different keys |

## Constraints

- Do not change nested product rules, 80% bars, or marquee top-level-only.
- Do not reuse Infini’s paint quadtree on device (nested overlapping AABBs).
- Do not rebuild the index during live move/resize.
- Do not approximate 80% from AABB overlap.

## Out of scope

Stories, application code, Master Plan / board, EP-070…073, TRACK-006, Path A toolbar, Device Settings, REQ-15, REQ-08, CHL-0011 reopen.
