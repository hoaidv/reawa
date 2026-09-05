---
id: STORY-EP-078
title: Spatial R-tree and named geometry queries
kind: implement
parent_srs: [SRS-EP-79, SRS-EP-78]
parent_req: [REQ-04]
status: draft
priority: P0
iter: iter-005
estimate: 8
owner: dev
depends_on: [STORY-EP-074]
acceptance_criteria:
  - "Given a DeviceDocument after op commit, When the spatial index is inspected, Then it is one R-tree of composed, ancestor-clipped world AABBs with paint-rank and depth (not local bounds, not Infini’s paint quadtree)."
  - "Given the 500-node product fixture, When each named query in [SRS-EP-79](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries) runs, Then hit ids match the nested linear walk 100% and R-tree probes are ≤ 64·⌈log₂(n)⌉ + k (k = overlapping AABBs, not n)."
  - "Given the 5000-node stress fixture, When a point query runs, Then p95 wall clock is ≤100 ms and a probe count that tracks n linearly fails even if wall clock is under 100 ms."
  - "Given a live move or resize preview, When frames paint, Then the index is not written (0 live-preview rebuilds) and ink p95 stays ≤30 ms on the 500-node fixture."
  - "Given op commit / accepted doc_load / undo apply, When the index rebuilds, Then it is off the ink/paint thread; p95 rebuild ≤5 ms at n=500 and ≤50 ms at n=5000."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-078 — Spatial R-tree and named geometry queries

Owns the **index**, not the caller migrations. [SRS-EP-79](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries)
named API; [SRS-EP-78](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-78-log-hit-test)
complexity bars; [ADR-0040](../../../.docs/adr/ADR-0040-logarithmic-hit-test.md) (`proposed`).

Product hit rules stay in [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation)
/ [SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent)
— this story does **not** change them. Exact 80% stays on the candidate set k.

**Queued** behind nested tap [STORY-EP-074](./STORY-EP-074.md) (in-review). Do not implement
against the old top-level-only walk. Do not start until the human picks this wave versus
field-latency [STORY-EP-070](./STORY-EP-070.md)…[STORY-EP-072](./STORY-EP-072.md).
No design package. No new chrome.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-074](./STORY-EP-074.md) Nested ink-box RenderingContext and tap-select |

## Named queries this story must expose

Point · Rect ≥80% · Polygon ≥80% · Highest-paint container ≥80% · Draw-into membership ·
Enclose capture · AABB overlap cull. Callers still linear until [STORY-EP-079](./STORY-EP-079.md)
/ [STORY-EP-080](./STORY-EP-080.md).

## Done when

- One STR R-tree; probe counters on the query helper (not in paint)
- 500- and 5000-node fixtures make O(n) vs O(log n + k) distinguishable
- 0 live-preview index writes; ink budget unstolen
- Quality Assurance Engineer can tag a probe-count scenario `@SRS-EP-78`
