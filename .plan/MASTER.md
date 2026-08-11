---
updated: 2026-08-11
current_iter: iter-003
owner: sm

# Campaign: Smart Group pilot (post Infini↔Epaper Must sync).
execution:
  direction: vertical
  scope:
    modules: [infini, epaper]
    features:
      - infini/vector-document
      - epaper/tool-modes
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
| Direction | **vertical** | Two features in flight (`wip: 2`) — the ink-box needs both ends |
| Scope | `infini` · vector-document **+** `epaper` · tool-modes | Human expanded 2026-08-11 (see below) |
| Stop line | **verified** | Design + implement through verify |
| Autonomy | **bounded** | Out-of-scope → backlog |

**Scope expansion (2026-08-11, human-approved).** The adopted ink-box UX puts a three-tool
toolbar (`Selection · Pen · Ink-box`) on the tablet, so `epaper` joins the campaign and WIP goes
to 2. Recorded in [epaper REQ-03](../.docs/modules/epaper/prd.md#tool-modes) and
[infini REQ-04](../.docs/modules/infini/prd.md#smart-group); rationale in the
[PM → architect handoff](./iter-003/handoffs/2026-08-11-pm-to-architect-ink-box-ux.md).

**Exit criteria:** REQ-04 Smart Group pilot **and** epaper REQ-03 tool modes accepted by human
(AC from the thickened PRDs); design stories done for both (`Needs design: yes` on each).

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-10 | Epaper on-device pan/zoom | backlog |
| 2026-08-10 | `reawa/*` | backlog |
| 2026-08-11 | STORY-IN-006 DocChrome | cancelled |
| 2026-08-11 | `doc_op` / regionsync migration | backlog — **but** tool-intent transport is now in scope (REQ-04 open question) |
| 2026-08-11 | Rotation + connectors on a Smart Group | backlog (REQ-04 Non-Goal, pilot) |
| 2026-08-11 | Mouse ink drawing on Infini (`Pen` on desktop) | backlog (REQ-04 Non-Goal, pilot) |
| 2026-08-11 | In-box content alignment / reflow | backlog (REQ-04 Non-Goal, pilot) |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **closed** — Must W4+W5 gated READY-WITH-CONCERNS | IN-010 → iter-003 | [iter](./iter-002/iter.md) · [retro](./iter-002/retro.md) |
| iter-003 | Smart Group pilot | **active** — PM adopted ink-box UX; scope expanded to epaper | — | [iter](./iter-003/iter.md) |

## Now — iter-003

### Goal & capacity

- Goal: ink-box pilot across both ends — tool-armed enclose + explicit selection, move/resize,
  `inkScaleMode`; tablet gets a 3-tool toolbar.
- Capacity: 11 stories sliced (~47 pts committed vs 15 capacity — sequence W3→W6; pull by wave).
- Risks: **three prerequisites** (ink ingestion, selection/hit-testing, undo); RM2 touch unverified; two features in flight.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **done** | — | [track](./tracks/TRACK-002-infini-vector-document.md) |
| TRACK-003 | planned | **paused** | **CHL-0008** — architecture rework; await PM + `/architect` | [track](./tracks/TRACK-003-smart-group-pilot.md) |

### Open challenges / blocked

- **CHL-0008 open (high)** — human restored code to HEAD; total architecture rework. Interrupts TRACK-003.
- CHL-0001…0003 adopted earlier; CHL-0004…0007 are plan residue (resolved on paper) but **code hotfixes discarded** by restore — do not treat as shipped.
- IN-006 remains cancelled (historical).

### Design packages in flight

- [ink-box-ui](./iter-003/design/ink-box-ui/) — STORY-IN-013 **ready**
- [epaper-tool-strip](./iter-003/design/epaper-tool-strip/) — STORY-EP-003 draft (spike-gated)

### Execution board(s)

- [iter-003 execution-board](./iter-003/execution-board.md) — **PAUSED (CHL-0008)**
- [iter-002 board (final)](./iter-002/execution-board.md) — frozen

### Freeze notes

- **TRACK-003 paused** — see track freeze note. No `/dev` on verify-fix or CHL-0007 paths.
- Retro-gate: [pm-retro-gate-pass](./iter-002/handoffs/2026-08-11-pm-retro-gate-pass.md)

## Forward

- **Now:** `/pm` triage CHL-0008 → re-lock execution → `/architect` redesign → `/sm` re-slice
- Backlog: [backlog.md](./backlog.md)
