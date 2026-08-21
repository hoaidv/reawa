---
updated: 2026-08-21
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
| WIP | **2** | WAIT Product Manager adopt [ADR-0032](../.docs/adr/ADR-0032-inverse-op-undo.md) (proposed). **No code.** W3 / Device Settings still frozen. |
| Validated | — | Follow toggles EP-055 + IN-037 **done**; EP-038 + EP-039 **done**; **hand-touch human-approved** 2026-08-20 (20 mm / HT); IN-033 **done** (host); Device Settings on-device (REQ-20 / ADR-0031) |

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

- Goal: **Hand-on-paper** plus **viewport follow** (human 2026-08-20). Cameras independent by default.
- Capacity: committed stories include EP-053…058 / IN-036…037. [STORY-IN-033](./iter-005/stories/STORY-IN-033.md) **done**. Human 2026-08-21: whole-tree snapshot undo is **wrong**; inverse-op undo will change **in this track** ([CHL-0026](./iter-005/challenges/CHL-0026-inverse-op-undo.md)). **No code** until the Architecture Decision Record is accepted.
- Risks: [CHL-0022](./iter-005/challenges/CHL-0022-shipped-no-device-pan.md); [CHL-0026](./iter-005/challenges/CHL-0026-inverse-op-undo.md); remaining Infini follow field test; device/Qt `epaper_bin` not built in this environment.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001…004 | planned | **done** | — | [tracks](./tracks/) |
| TRACK-005 | planned | **active** | WAIT Product Manager adopt ADR-0032 (CHL-0026). No code. Follow field test still outstanding. | [track](./tracks/TRACK-005-hand-on-paper.md) |

### Open challenges / blocked

- CHL-0011 / CHL-0012 / REQ-08 **not this lock**.
- [CHL-0022](./iter-005/challenges/CHL-0022-shipped-no-device-pan.md) (Shipped “no device pan / no arrowheads” prose vs TRACK-005) — open; implement against new ids until Product Manager adopts.
- [CHL-0026](./iter-005/challenges/CHL-0026-inverse-op-undo.md) (Inverse-op undo, not whole-tree snapshots) — open; **this track**; no code until Architecture Decision Record accept.
- EP-032 parked in iter-004.

### Design packages in flight

- [hand-touch](./iter-005/design/hand-touch/) — EP-037 + EP-054 **done**; UI-EP-06 amended 2026-08-20 (20 mm + HT)
- [pen-button-map](./iter-005/design/pen-button-map/) — EP-056 **done** ([UI-EP-08](./iter-005/design/pen-button-map/ui-spec.md)); UI-IN-03 superseded
- [viewport-follow-epaper](./iter-005/design/viewport-follow-epaper/) — EP-053 **done** (UI-EP-07)
- [viewport-follow-infini](./iter-005/design/viewport-follow-infini/) — IN-036 **done** (UI-IN-04)
- (queued) erase-chrome, clipboard-chrome, connector-ends, connector-attach, manual-create

### Execution board(s)

- [iter-005 execution-board](./iter-005/execution-board.md) — **WAIT** Product Manager adopt [ADR-0032](../.docs/adr/ADR-0032-inverse-op-undo.md). W3 still frozen. Follow field test still outstanding.

### Freeze notes

- TRACK-004 **done**. Gate: [pm-retro-gate-pass](./iter-004/handoffs/2026-08-16-pm-retro-gate-pass.md).
- TRACK-005 field-test pause 2026-08-20: [sm-to-human-field-test](./iter-005/handoffs/2026-08-20-sm-to-human-field-test.md). Hand-touch join: [sm-to-human-hand-touch-verified](./iter-005/handoffs/2026-08-20-sm-to-human-hand-touch-verified.md). **Unpaused for docs 2026-08-21:** inverse-op undo ([CHL-0026](./iter-005/challenges/CHL-0026-inverse-op-undo.md)). W3 / application code still frozen.

## Forward

- Inverse-op undo: [ADR-0032](../.docs/adr/ADR-0032-inverse-op-undo.md) **proposed**. Next: Product Manager adopt / Product Requirements Document thicken, then Software Requirements Specification bind, then Scrum Master slices. **No code** until then.
- After remaining follow field test: say go in `/sm`. Then W3 erase design **or** Device Settings scenarios — human picks. Do **not** start those while the undo Architecture Decision Record is in flight unless the human says so.
- Parked: REQ-15, REQ-08, CHL-0011, CHL-0012, EP-035 measure.
- Backlog: [backlog.md](./backlog.md)
