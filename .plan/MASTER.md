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
| 2026-08-11 | vector-document `/dev` before sync | deferred → W4 with tablet/region sync |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **active** — F1 verified; F2 arch | design cancelled | [iter](./iter-002/iter.md) |

## Now — iter-002

### Goal & capacity

- Goal: Architect prepares vector-document model for **epaper↔Infini sync**; no designer/dev on F2 chrome yet.
- Capacity: architecture only (W3-arch).
- Risks: SRS orphans until W4 implement; Smart Group Could rides with sync.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **active** | **`/architect` sync readiness** | [track](./tracks/TRACK-002-infini-vector-document.md) |

### Open challenges / blocked

- STORY-IN-006 **blocked** (design cancelled by human).

### Design packages in flight

- [infinity-canvas](./iter-002/design/infinity-canvas/) — STORY-IN-001 **done**
- [vector-document](./iter-002/design/vector-document/) — **not painting** (IN-006 cancelled)

### Execution board(s)

- [execution-board](./iter-002/execution-board.md) — wave **W3-arch**

### Freeze notes

- **No `/dev`** on vector-document / Smart Group until sync wave (W4).
- **No `/designer`** on STORY-IN-006.

## Forward

- W4: implement document tree + SVG/ops + Smart Group **together with** tablet-sync / region-sync.
- Backlog: [backlog.md](./backlog.md)
