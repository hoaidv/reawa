---
updated: 2026-08-16
current_iter: iter-005
owner: sm

# Campaign: TRACK-005 hand-on-paper (REQ-10…18 except REQ-15). Vertical · verified · wip 2.
execution:
  direction: vertical
  scope:
    modules: [epaper, infini]
    features:
      - epaper/ink-box
      - epaper/tool-modes
      - epaper/connector-ink
      - epaper/region-sync
      - epaper/local-pen-ink
      - epaper/device-document
      - infini/infinity-canvas
      - infini/tablet-sync
      - infini/vector-document
  stop_line: verified
  autonomy: bounded
  out_of_scope: backlog
  wip: 2
  validated_by: ""
---

# Master Plan

Orientation: **history spine (thin)** → **Now (thick)** → **forward (thin)**.
Product truth in `.docs/`. Skill: [`execution-lock`](../.agent/personas/shared/execution-lock.md).

## Execution lock

| Field | Value | Why |
|---|---|---|
| Direction | **vertical** | One campaign: hand-on-paper wave end-to-end |
| Scope | epaper ink-box, tool-modes, connector-ink, region-sync, local-pen-ink, device-document; infini canvas, tablet-sync, vector-document | REQ-10…14, 17, 18 + infini REQ-05 |
| Stop line | **verified** | design → BDD → implement → human confirm |
| Autonomy | **bounded** | Run inside lock; sink REQ-15 / REQ-08 |
| WIP | **2** | W1: EP-037 ∥ IN-034 |
| Validated | — | Campaign just opened |

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-16 | Table recognition REQ-15 | backlog — human excluded from TRACK-005 |
| 2026-08-16 | REQ-16 as separate id | retired → REQ-10 |
| 2026-08-13 | Generic any-node manipulation | REQ-08 parked |
| 2026-08-14 | Nested enclose / FREE_FORM | CHL-0011 / CHL-0012 backlog |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **closed** | IN-010 → iter-003 | [iter](./iter-002/iter.md) |
| iter-003 | Epaper owns the document | **closed** | REQ-08 parked | [iter](./iter-003/iter.md) |
| iter-004 | On-device connectors + ToolChip | **closed** — verified 2026-08-16 | EP-035 parking | [iter](./iter-004/iter.md) · [retro](./iter-004/retro.md) |

## Now — iter-005

### Goal & capacity

- Goal: **Hand-on-paper** — REQ-10 (finger pick/move + pan/zoom), erase, clipboard, connector ends + attachments, barrel buttons, manual create. **Not** REQ-15.
- Capacity: ~77 pts committed, all `draft`. W0 is architect.
- Risks: SRS not bound; BRD-07 still defers on-device pan (EP-039 slice).

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001…004 | planned | **done** | — | [tracks](./tracks/) |
| TRACK-005 | planned | **active** | **W0 `/architect`** then `/designer` EP-037 ∥ IN-034 | [track](./tracks/TRACK-005-hand-on-paper.md) |

### Open challenges / blocked

- CHL-0011 / CHL-0012 / REQ-08 **not this lock**.
- EP-032 parked in iter-004.

### Design packages in flight

- [hand-touch](./iter-005/design/hand-touch/) — EP-037
- [pen-button-map](./iter-005/design/pen-button-map/) — IN-034
- (queued) erase-chrome, clipboard-chrome, connector-ends, connector-attach, manual-create

### Execution board(s)

- [iter-005 execution-board](./iter-005/execution-board.md) — **NOW W0** `/architect`

### Freeze notes

- TRACK-004 **done**. Gate: [pm-retro-gate-pass](./iter-004/handoffs/2026-08-16-pm-retro-gate-pass.md).

## Forward

- After W0: **`/designer`** EP-037 ∥ IN-034, then `/qa` / `/dev` EP-038.
- Parked: REQ-15, REQ-08, CHL-0011, CHL-0012, EP-035 measure.
- Backlog: [backlog.md](./backlog.md)
