---
iter: iter-002
goal: "Infini infinity canvas + epaper sync (EXP-0001 productization)"
start: 2026-08-10
end: 2026-08-24
capacity: 20
committed_points: 20
status: active
---

# Iter 002 — Infini + Epaper sync

Vertical campaign ([MASTER](../MASTER.md)): F1 `infinity-canvas` in flight (`wip: 1`).

## Committed

### NOW — F1 infinity-canvas (15 pts)

- [STORY-IN-001](./stories/STORY-IN-001.md) — design — designer — 3 pts — **ready**
- [STORY-IN-002](./stories/STORY-IN-002.md) — implement — dev — 3 pts — draft — depends_on STORY-IN-001
- [STORY-IN-003](./stories/STORY-IN-003.md) — implement — dev — 5 pts — draft — depends_on STORY-IN-001, STORY-IN-002
- [STORY-IN-004](./stories/STORY-IN-004.md) — implement — dev — 5 pts — draft — depends_on STORY-IN-001, STORY-IN-003
- [STORY-IN-005](./stories/STORY-IN-005.md) — implement — dev — 2 pts — draft — depends_on STORY-IN-004

### Queued (not WIP) — F2 design (2 pts)

- [STORY-IN-006](./stories/STORY-IN-006.md) — design — designer — 2 pts — **draft** — do not `in-progress` until F1 clears

## Carry-over candidates

- BDD backfill for iter-000 (optional)
- Epaper on-device pan/zoom (deferred Non-Goal)

## Risks

- Electron trackpad pinch performance (ADR-0008 spike via STORY-IN-005)
- Design thicken may block marking implement `ready`

## Links to product docs

- [Infini PRD](../../.docs/modules/infini/prd.md) · [Epaper PRD](../../.docs/modules/epaper/prd.md)
- [ADR-0008](../../.docs/adr/ADR-0008-electron-react-infini.md) · [ADR-0009](../../.docs/adr/ADR-0009-shared-document-viewport.md)
- Track: [TRACK-001](../tracks/TRACK-001-infini-infinity-canvas.md)

## Execution board

- [execution-board.md](./execution-board.md)
