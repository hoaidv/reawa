---
updated: 2026-08-10
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
| 2026-08-10 | STORY-IN-006 early parallel | blocked by wip — keep draft |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **active** — F1 in flight | — | [iter](./iter-002/iter.md) |

## Now — iter-002

### Goal & capacity

- Goal: Vertical F1 — Infini infinity canvas (REQ-01).
- Capacity / committed: **20 pts** (15 F1 + 2 queued design + buffer).
- Risks: Electron gesture budget; design gate before implement `ready`.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | active | **STORY-IN-002 /qa** | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |

### Open challenges / blocked

- None. Implement UI stories draft until STORY-IN-001 `done`.

### Design packages in flight

- [infinity-canvas](./iter-002/design/infinity-canvas/) — STORY-IN-001 **done** `[UI-IN-01]`
- [vector-document](./iter-002/design/vector-document/) — STORY-IN-006 queued (draft)

### Execution board(s)

- [execution-board](./iter-002/execution-board.md) — wave **W2-qa-dev**

### Freeze notes

- None.

## Forward

- After F1: F2 vector-document (STORY-IN-006 + implement slices) → tablet/region sync.
- Backlog: [backlog.md](./backlog.md)
