---
updated: 2026-08-11
current_iter: iter-002
owner: sm

# Campaign: Infini + Epaper sync (EXP-0001 productization).
execution:
  direction: vertical
  scope:
    modules: [infini, epaper]
    features:
      - infini/infinity-canvas
      - infini/vector-document
      - infini/tablet-sync
      - epaper/region-sync
  stop_line: verified
  autonomy: bounded
  out_of_scope: backlog
  wip: 1
  validated_by: ""
---

# Master Plan

Orientation: **history spine (thin)** → **Now (thick)** → **forward (thin)**.
Product truth in `.docs/`. Skill: [`execution-lock`](../.agent/personas/shared/execution-lock.md).

## Execution lock

| Field | Value | Why |
|---|---|---|
| Direction | **vertical** | Phases 1→2→3; one feature in flight (`wip: 1`) |
| Scope | `infini` + `epaper` · 4 features | Canvas → document → sync |
| Stop line | **verified** | Design + implement through verify |
| Autonomy | **bounded** | Out-of-scope → backlog |

**Exit criteria:** REQ-01…03 + Epaper REQ-02 verified on hardware; human `validated_by`.

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-10 | Epaper on-device pan/zoom | backlog |
| 2026-08-10 | `reawa/*` | backlog |
| 2026-08-11 | STORY-IN-006 design wave | **cancelled** — no DocChrome design |
| 2026-08-11 | vector-document `/dev` before sync | superseded — W4 opened with sync |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **active** — F1 verified; **W4** implement | IN-006 cancelled | [iter](./iter-002/iter.md) |

## Now — iter-002

### Goal & capacity

- Goal: Implement document tree + SVG/ops **together with** Infini tablet-sync and Epaper region-sync.
- Capacity: W4 Must chain (IN-007…009 + EP-001); Smart Group Could (IN-010) after.
- Risks: reconnect snapshot/hello TBD; dual fixtures in IN-008.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **active** | **`/qa` BDD → `/dev` IN-007** | [track](./tracks/TRACK-002-infini-vector-document.md) |

### Open challenges / blocked

- STORY-IN-006 **blocked** (design cancelled by human) — do not revive.

### Design packages in flight

- [infinity-canvas](./iter-002/design/infinity-canvas/) — STORY-IN-001 **done**
- [vector-document](./iter-002/design/vector-document/) — **not painting** (IN-006 cancelled)

### Execution board(s)

- [execution-board](./iter-002/execution-board.md) — wave **W4**

### Freeze notes

- **No `/designer`** on STORY-IN-006.
- `/dev` starts only after `/qa` BDD for the story in flight (order: IN-007 → …).

## Forward

- Finish W4 Must → IN-010 Could → hardware `validated_by`.
- Backlog: [backlog.md](./backlog.md)
