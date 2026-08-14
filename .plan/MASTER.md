---
updated: 2026-08-14
current_iter: iter-004
owner: sm

# Campaign: closed 2026-08-14 (device-document / ink-box). Next PRD from human.
execution:
  direction: vertical
  scope:
    modules: [epaper, infini]
    features: []
  stop_line: srs-ready
  autonomy: ask
  out_of_scope: backlog
  wip: 1
  validated_by: "human 2026-08-14"
---

# Master Plan

Orientation: **history spine (thin)** → **Now (thick)** → **forward (thin)**.
Product truth in `.docs/`. Skill: [`execution-lock`](../.agent/personas/shared/execution-lock.md).

## Execution lock

| Field | Value | Why |
|---|---|---|
| Direction | **vertical** | Hold until the next PRD; do not start REQ-08 by inertia |
| Scope | `epaper`, `infini` · **no features** | Awaiting human PRD |
| Stop line | **srs-ready** | No implement until PRD → SRS |
| Autonomy | **ask** | Confirm before slicing |

**Campaign closed 2026-08-14 (human).** Exit criteria for Epaper-owns-document **met**: REQ-04…07 + design for REQ-05/06; W12 human confirm. `[REQ-08]`, CHL-0011, CHL-0012 **not** in the next lock until the new PRD says so.

**Re-lock (2026-08-13, historical — CHL-0008).** Device owned the in-session document; Infini viewer + persistence; one-way sync. See [iter-003 retro](./iter-003/retro.md).

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-10 | Epaper on-device pan/zoom | backlog |
| 2026-08-10 | `reawa/*` | backlog |
| 2026-08-11 | STORY-IN-006 DocChrome | cancelled |
| 2026-08-11 | Rotation + connectors on a Smart Group | backlog → now folded into epaper `[REQ-08]` (next campaign) |
| 2026-08-11 | Mouse ink drawing on Infini (`Pen` on desktop) | backlog |
| 2026-08-11 | In-box content alignment / reflow | backlog |
| 2026-08-13 | Desktop-side ink-box authoring (Infini Selection / Ink-box tools) | backlog — infini `[REQ-04]` deprecated until multi-directional sync |
| 2026-08-13 | Multi-directional sync / modern doc-sync algorithm / CRDT | backlog — explicitly deferred by human |
| 2026-08-13 | On-device persistence, offline work, sync-any-moment | backlog — explicitly deferred by human |
| 2026-08-13 | Generic manipulation of any document node | epaper `[REQ-08]` — thickened now, **distinct iteration** |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **closed** — Must W4+W5 gated READY-WITH-CONCERNS | IN-010 → iter-003 | [iter](./iter-002/iter.md) · [retro](./iter-002/retro.md) |
| iter-003 | Epaper owns the document (REQ-04…07) | **closed** — W12 human confirm; campaign exited | REQ-08 / CHL-0011 / CHL-0012 await PRD | [iter](./iter-003/iter.md) · [retro](./iter-003/retro.md) |

## Now — iter-004

### Goal & capacity

- Goal: **Awaiting human PRD.** No stories committed. Do not auto-slice REQ-08 / CHL-0011 / CHL-0012.
- Capacity: unset until PRD.
- Risks: starting the next Must from leftover thickening instead of the new brief.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **done** | — | [track](./tracks/TRACK-002-infini-vector-document.md) |
| TRACK-003 | planned | **done** | campaign exited 2026-08-14 | [track](./tracks/TRACK-003-smart-group-pilot.md) |

### Open challenges / blocked

- CHL-0011 / CHL-0012 **future** — not in lock until PRD includes them.
- ADR-0019 / CHL-0018 amend **deferred**.
- Residue EP-007…011 / IN-020…026 remain **blocked** on iter-003.

### Design packages in flight

_none — iter-003 packages are the current index winners (UI-EP-01/02/03)._

### Execution board(s)

- [iter-003 execution-board](./iter-003/execution-board.md) — **frozen** (W12 done, campaign closed)
- [iter-002 board (final)](./iter-002/execution-board.md) — frozen

### Freeze notes

- TRACK-003 **done**. Retro: [iter-003/retro.md](./iter-003/retro.md). Gate: [pm-retro-gate-pass](./iter-003/handoffs/2026-08-14-pm-retro-gate-pass.md).

## Forward

- **Now:** `/pm` with the new PRD (human will supply). Then `/architect` → `/sm` slice.
- Parked (not committed): epaper `[REQ-08]`, nested enclose (CHL-0011), FREE_FORM / align-content (CHL-0012).
- Backlog: [backlog.md](./backlog.md)
