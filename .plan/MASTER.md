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
  validated_by: "human / 2026-08-11 — W4 Must draw sync (RM2→Infini) confirmed; READY-WITH-CONCERNS see pm-gate-review-w4"
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
| iter-002 | Infini + sync | **active** — **W5** pan/zoom→tablet (W4 Must gated) | IN-006 cancelled; IN-010 parked | [iter](./iter-002/iter.md) |

## Now — iter-002

### Goal & capacity

- Goal: **W5** live ADR-0009 viewport (marker + coalesce + stroke scale); draw anywhere via pan/zoom.
- Capacity: IN-010 Could stays parked; no DocChrome design.
- Risks: stroke Desktop↔Tablet parity; e-ink refresh budget; reconnect TBD.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **active** | **`/architect` W5 thicken → /qa → /dev IN-011→EP-002** | [track](./tracks/TRACK-002-infini-vector-document.md) |

### Open challenges / blocked

- STORY-IN-006 **blocked** (design cancelled by human) — do not revive.

### Design packages in flight

- [infinity-canvas](./iter-002/design/infinity-canvas/) — STORY-IN-001 **done**
- [vector-document](./iter-002/design/vector-document/) — **not painting** (IN-006 cancelled)

### Execution board(s)

- [execution-board](./iter-002/execution-board.md) — **W5 NOW**; W4 gated

### Freeze notes

- **No `/designer`** on STORY-IN-006; no design stories for W5 marker (REQ-03 Needs design: no).
- Gate W4: [pm-gate-review-w4](./iter-002/handoffs/2026-08-11-pm-gate-review-w4.md)
- W5 open: [sm-to-architect-w5](./iter-002/handoffs/2026-08-11-sm-to-architect-w5.md)

## Forward

- Optional: STORY-IN-010 Smart Group Could (Needs design: yes — plan design story if opened).
- Follow-up: Qt `RegionSession` wire; reconnect snapshot/hello.
- Backlog: [backlog.md](./backlog.md)
