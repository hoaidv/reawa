---
iter: iter-002
goal: "Infini infinity canvas + epaper sync (EXP-0001 productization)"
start: 2026-08-10
end: 2026-08-24
capacity: 20
committed_points: 41
status: active
---

# Iter 002 — Infini + Epaper sync

Vertical campaign ([MASTER](../MASTER.md)): wave **W4** implement (`wip: 1`).

## Committed

### Done — F1 infinity-canvas (15 pts)

- [STORY-IN-001](./stories/STORY-IN-001.md) — design — designer — 3 pts — **done**
- [STORY-IN-002](./stories/STORY-IN-002.md) — implement — dev — 3 pts — **done**
- [STORY-IN-003](./stories/STORY-IN-003.md) — implement — dev — 5 pts — **done**
- [STORY-IN-004](./stories/STORY-IN-004.md) — implement — dev — 5 pts — **done**
- [STORY-IN-005](./stories/STORY-IN-005.md) — implement — dev — 2 pts — **done**

### Cancelled — DocChrome design

- [STORY-IN-006](./stories/STORY-IN-006.md) — design — designer — 2 pts — **blocked** (cancelled)

### NOW — W4 sync implement (21 pts Must + 3 Could)

- [STORY-IN-007](./stories/STORY-IN-007.md) — implement — dev — 5 pts — **ready** — SRS-IN-04
- [STORY-IN-008](./stories/STORY-IN-008.md) — implement — dev — 3 pts — **ready** — depends_on IN-007
- [STORY-IN-009](./stories/STORY-IN-009.md) — implement — dev — 5 pts — **ready** — depends_on IN-007, IN-008
- [STORY-EP-001](./stories/STORY-EP-001.md) — implement — dev — 5 pts — **ready** — depends_on IN-009
- [STORY-IN-010](./stories/STORY-IN-010.md) — implement — dev — 3 pts — **draft** — Smart Group Could

## Carry-over candidates

- BDD backfill for iter-000 (optional)
- Epaper on-device pan/zoom (deferred Non-Goal)
- Reconnect snapshot/hello (architect TBD)

## Risks

- Dual TS+Qt fixtures (IN-008) before sync ship
- Reconnect catch-up unspecified — do not block Must ACs

## Links to product docs

- [Infini PRD](../../.docs/modules/infini/prd.md) · [Epaper PRD](../../.docs/modules/epaper/prd.md)
- [ADR-0009](../../.docs/adr/ADR-0009-shared-document-viewport.md) · [ADR-0010](../../.docs/adr/ADR-0010-tree-of-vectors.md) · [ADR-0011](../../.docs/adr/ADR-0011-smart-group.md)
- Track: [TRACK-002](../tracks/TRACK-002-infini-vector-document.md)

## Execution board

- [execution-board.md](./execution-board.md)
