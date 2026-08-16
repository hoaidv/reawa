---
updated: 2026-08-16
current_iter: iter-005
owner: sm

# Campaign closed: TRACK-004. Iter-005 empty until human picks the next wave.
execution:
  direction: vertical
  scope:
    modules: [epaper, infini]
    features: []
  stop_line: srs-ready
  autonomy: ask
  out_of_scope: backlog
  wip: 0
  validated_by: "2026-08-16 human — TRACK-004 / iter-004"
---

# Master Plan

Orientation: **history spine (thin)** → **Now (thick)** → **forward (thin)**.
Product truth in `.docs/`. Skill: [`execution-lock`](../.agent/personas/shared/execution-lock.md).

## Execution lock

| Field | Value | Why |
|---|---|---|
| Direction | **vertical** | Next campaign TBD — do not start a second feature |
| Scope | `epaper`, `infini` — **no features locked** | Human picks the wave |
| Stop line | **srs-ready** | No design/implement in-progress until PRD/SRS for the wave |
| Autonomy | **ask** | Empty iter; confirm before slicing |
| WIP | **0** | EP-035 is carry-over `ready`, not in flight |
| Validated | 2026-08-16 human | TRACK-004 on-device connectors verified |

**Closed campaign (TRACK-004):** REQ-09 + REQ-03 ToolChip human-confirmed; Infini live mirror. EP-036 gadget-restore **cancelled** (Linux USB inspect). EP-035 enclose A/L **carried**, not the next Must.

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-16 | Software USB re-enum / gadget restore without unplug | **cancelled** EP-036 — see [memory](../.docs/memory/rm2-usb-gadget-no-software-unplug.md) |
| 2026-08-13 | Generic manipulation of any document node | epaper `[REQ-08]` |
| 2026-08-14 | Nested enclose (CHL-0011), FREE_FORM / align-content (CHL-0012) | backlog |
| 2026-08-14 | Guard-corpus false-positive bar (EXP-0002 Initiative 2) | ship gate |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **closed** — Must W4+W5 gated READY-WITH-CONCERNS | IN-010 → iter-003 | [iter](./iter-002/iter.md) · [retro](./iter-002/retro.md) |
| iter-003 | Epaper owns the document (REQ-04…07) | **closed** — W12 human confirm; campaign exited | REQ-08 / CHL-0011 / CHL-0012 stay parked | [iter](./iter-003/iter.md) · [retro](./iter-003/retro.md) |
| iter-004 | On-device connectors + ToolChip | **closed** — human verified 2026-08-16 | EP-035 → iter-005; EP-036 cancelled | [iter](./iter-004/iter.md) · [retro](./iter-004/retro.md) |

## Now — iter-005

### Goal & capacity

- Goal: **empty shell**. Human will pick the next feature wave and improvements. [STORY-EP-035](./iter-004/stories/STORY-EP-035.md) sits as one small enhancement — **not committed** until the wave is chosen.
- Capacity: 0 pts committed.
- Do **not** `/dev` EP-035 yet. Do **not** slice BS-0002 REQs until `/pm` names Musts.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **done** | — | [track](./tracks/TRACK-002-infini-vector-document.md) |
| TRACK-003 | planned | **done** | — | [track](./tracks/TRACK-003-smart-group-pilot.md) |
| TRACK-004 | planned | **done** | campaign verified 2026-08-16 | [track](./tracks/TRACK-004-on-device-connectors.md) |

### Open challenges / blocked

- CHL-0011 / CHL-0012 **future**.
- epaper `[REQ-08]` **future**.
- EP-032 chrome ADR **parked** in iter-004 (`draft`).
- Residue EP-007…011 / IN-020…026 remain **blocked** on iter-003.

### Design packages in flight

- _none_ — UI-EP-04 / UI-EP-05 stay current in iter-004 packages.

### Execution board(s)

- [iter-005](./iter-005/iter.md) — no board until a lock + wave exists
- [iter-004 execution-board](./iter-004/execution-board.md) — **frozen**

### Freeze notes

- TRACK-004 **done**. Retro: [iter-004/retro.md](./iter-004/retro.md). Gate: [pm-retro-gate-pass](./iter-004/handoffs/2026-08-16-pm-retro-gate-pass.md).

## Forward

- Human picks next wave (notes: [BS-0002](./iter-004/brainstorms/BS-0002-iter-005-feature-wave.md), draft REQs 11–18 / infini REQ-05). Then `/pm` → `/architect` → `/sm` slice. Include EP-035 as a small enhancement beside the Musts.
- Parked: REQ-08, CHL-0011, CHL-0012, EP-032.
- Backlog: [backlog.md](./backlog.md)
