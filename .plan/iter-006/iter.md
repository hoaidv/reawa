---
iter: iter-006
goal: "A single-threaded then concurrent document forest in the rebuilt Epaper: pen ink painted from the forest, with readers safe beside the writer (Milestones A and B)."
start: 2026-10-04
end: 2026-10-31
capacity: 32
committed_points: 32
status: active
---

# Iter 006 — Document forest

Track: [TRACK-009](../tracks/TRACK-009-document-forest.md) (Document forest) ·
paused [TRACK-007](../tracks/TRACK-007-follow-through.md) ·
Board: [execution-board-document-forest](./execution-board-document-forest.md) ·
Plan: [document-forest-implementation](../document-forest-implementation.md)

Lock: **vertical · verified · wip 1 · ask**. The human writes the code; agents guide and review.
Opened 2026-10-04 after the [iter-005 retro gate](../iter-005/handoffs/2026-10-04-pm-retro-gate-pass.md).
The end date and capacity are first guesses for a solo developer; re-baseline at the first review.

## Committed

Milestone A: single-threaded forest.
- [STORY-EP-082](./stories/STORY-EP-082.md) — implement — dev — 3 pts — **ready** — forest foundations
- [STORY-EP-083](./stories/STORY-EP-083.md) — implement — dev — 3 pts — depends_on EP-082 — draft
- [STORY-EP-084](./stories/STORY-EP-084.md) — implement — dev — 5 pts — depends_on EP-083 — draft
- [STORY-EP-085](./stories/STORY-EP-085.md) — implement — dev — 5 pts — depends_on EP-084 — draft

Milestone B: concurrent.
- [STORY-EP-086](./stories/STORY-EP-086.md) — implement — dev — 8 pts — depends_on EP-085 — draft
- [STORY-EP-087](./stories/STORY-EP-087.md) — implement — dev — 8 pts — depends_on EP-086 — draft

## Planned on the track, not committed

- [STORY-EP-088](./stories/STORY-EP-088.md) manipulation · [STORY-EP-089](./stories/STORY-EP-089.md)
  connector · [STORY-EP-090](./stories/STORY-EP-090.md) progressive paint ·
  [STORY-EP-091](./stories/STORY-EP-091.md) undo. Committed when Milestone B is done or capacity allows.

## Carry-over candidates

- Paused [TRACK-007](../tracks/TRACK-007-follow-through.md) stories (EP-048…052, EP-057, EP-058,
  EP-070…073) stay in iter-005, not committed. Most target the archived application.
- [STORY-EP-045](../iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](../iter-005/stories/STORY-EP-046.md) blocked (frozen).

## Challenges

- [CHL-0033](./challenges/CHL-0033-forest-product-records.md) (Epaper product records under the forest) — **adopted**
- [CHL-0034](./challenges/CHL-0034-forest-wire-sync-deferred.md) (Forest ↔ wire and Infini sync) — **deferred**

## Risks

- Granule rules may cost too much to build; fallback is a reader–writer lock behind the same API (plan step 5).
- `adlc audit` does not scan `.cpp`, so story progress is not visible in the sync report.
- BRD-07 does not yet show Infini sync deferred (analyst follow-up).
- `epaper/` already holds pen and finger ink code with no story; EP-085 builds on it.

## Links to product docs

- Module PRD: [REQ-04](../../.docs/modules/epaper/prd.md#device-document) · [REQ-01](../../.docs/modules/epaper/prd.md#local-pen-ink)
- Feature SRS: [device-document](../../.docs/modules/epaper/features/device-document/index.md) —
  [SRS-EP-07](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document),
  [SRS-EP-80](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries),
  [SRS-EP-81](../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality)
- Design: [document-forest](../../.docs/domain/document-forest/index.md) · [ADR-0041](../../.docs/adr/ADR-0041-document-forest.md)

## Execution board

- [execution-board-document-forest.md](./execution-board-document-forest.md)
