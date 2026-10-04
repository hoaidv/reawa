---
updated: 2026-10-04
current_iter: iter-006
owner: sm

# Campaign: TRACK-009 document forest (ADR-0041) in the rebuilt Epaper. The human codes; agents guide. Vertical · verified · wip 1 · ask.
execution:
  direction: vertical
  scope:
    modules: [epaper]
    features:
      - epaper/device-document
      - epaper/local-pen-ink
  stop_line: verified
  autonomy: ask
  out_of_scope: backlog
  wip: 1
  # Human 2026-10-04: accept ADR-0041; Infini sync out of implementation scope (CHL-0034); the human writes the code. local-pen-ink only for STORY-EP-085.
  validated_by: ""
---

# Master Plan

Orientation: **history spine (thin)** → **Now (thick)** → **forward (thin)**.
Product truth in `.docs/`. Skill: [`execution-lock`](../.agent/personas/shared/execution-lock.md).

## Execution lock

| Field | Value | Why |
|---|---|---|
| Direction | **vertical** | One track, the document forest, through its milestones |
| Scope | epaper/device-document (+ epaper/local-pen-ink for [STORY-EP-085](./iter-006/stories/STORY-EP-085.md) only) | The forest realizes REQ-04; Milestone A rewires the existing pen path |
| Stop line | **verified** | Each story ends with named host tests or device measurements |
| Autonomy | **ask** | The human writes the code. Agents explain, review and check evidence; they do not edit `epaper/src/` |
| WIP | **1** | [STORY-EP-082](./iter-006/stories/STORY-EP-082.md) (Forest foundations) is **ready** |
| Validated | — | iter-005 retro gate passed 2026-10-04. ADR-0041 accepted 2026-10-04 (human). Earlier validations: see iter-005 history row |

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-16 | Table recognition REQ-15 | backlog — human excluded from TRACK-005 |
| 2026-08-16 | REQ-16 as separate id | retired → REQ-10 |
| 2026-08-13 | Generic any-node manipulation | REQ-08 parked |
| 2026-08-14 | Nested enclose / FREE_FORM | CHL-0011 **scheduled** 2026-09-05 via [CHL-0032](./iter-005/challenges/CHL-0032-nested-ink-box.md) (this lock, `epaper/ink-box`). CHL-0012 FREE_FORM still backlog |
| 2026-08-27 | DeviceMap invert user interface; Mouse DragHandler; further tool-system polish | backlog — TRACK-006 closed; do not continue unless a TRACK-007 story needs it |
| 2026-09-05 | TRACK-005 closed (too large); remainder opened as TRACK-007 | lock campaign header flipped; same feature scope; do not reopen TRACK-005 |
| 2026-08-27 | Infini apply undo (`compound` / `set_ink_samples`); whole tablet→desktop undo sync | backlog — [STORY-IN-038](./iter-005/stories/STORY-IN-038.md) cancelled; waits independent sync algorithm |
| 2026-10-03 | Hand-on-paper remainder (field latency, hit-test, barrel, attachments, manual create, Infini follow) | paused with [TRACK-007](./tracks/TRACK-007-follow-through.md) — human ordered [TRACK-008](./tracks/TRACK-008-rebuild-epaper.md) |
| 2026-10-04 | Infini connection and sync for the rebuilt Epaper (wire mapping, `doc_load`, forest ↔ wire) | deferred — [CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md); human kept the forest work focused |
| 2026-10-04 | Ink-box, selection, touch and manipulation tools on the forest | later tracks — records deprecated until ported ([CHL-0033](./iter-006/challenges/CHL-0033-forest-product-records.md)) |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **closed** | IN-010 → iter-003 | [iter](./iter-002/iter.md) |
| iter-003 | Epaper owns the document | **closed** | REQ-08 parked | [iter](./iter-003/iter.md) |
| iter-004 | On-device connectors + ToolChip | **closed** — verified 2026-08-16 | EP-035 parking | [iter](./iter-004/iter.md) · [retro](./iter-004/retro.md) |
| iter-005 | Hand-on-paper, then rebuild Epaper | **closed** 2026-10-04; retro-gate passed. Hand-touch, undo, erase, clipboard, Path B, nested ink-box human-verified; app archived to `epaper_old/`; ADR-0041 accepted | TRACK-007 paused stories; EP-045/046 frozen; EP-078…080 cancelled | [iter](./iter-005/iter.md) · [retro](./iter-005/retro.md) |

## Now — iter-006

### Goal & capacity

- Goal: **Document forest**, Milestones A and B: pen ink painted from the forest, then readers safe beside the writer ([iter](./iter-006/iter.md)).
- Capacity: 32 points committed (EP-082…087), end 2026-10-31, both first guesses for a solo developer. EP-088…091 planned on the track, not committed.
- Plan: [document-forest-implementation.md](./document-forest-implementation.md). Step 0 **done** 2026-10-04.
- Risks: granule rules may cost too much (fallback in plan step 5); `adlc audit` does not scan `.cpp`; `epaper/` holds pen and finger ink code with no story.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001…006 | — | **done** | — | [tracks](./tracks/) |
| TRACK-007 | planned | **paused** | Frozen 2026-10-03. Its stories carry over paused; most target the archived application | [track](./tracks/TRACK-007-follow-through.md) |
| TRACK-008 | expedite | **done** | Closed 2026-10-04. Continued by TRACK-009 | [track](./tracks/TRACK-008-rebuild-epaper.md) |
| TRACK-009 | planned | **active** | [STORY-EP-082](./iter-006/stories/STORY-EP-082.md) (Forest foundations) **ready** — the human codes | [track](./tracks/TRACK-009-document-forest.md) |

### Open challenges / blocked

- [CHL-0033](./iter-006/challenges/CHL-0033-forest-product-records.md) (Epaper product records under the forest) — **adopted** 2026-10-04.
- [CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md) (Forest ↔ wire and Infini sync) — **deferred** 2026-10-04.
- CHL-0022, CHL-0023, CHL-0027 — **deferred** 2026-10-04; they feed the successors of the deprecated ink-box records.
- CHL-0012 / REQ-08 — parked.

### Execution board(s)

- [iter-006 execution-board-document-forest](./iter-006/execution-board-document-forest.md) — TRACK-009, wave A1.
- iter-005 boards are archive: [rebuild-epaper](./iter-005/execution-board-rebuild-epaper.md), [follow-through](./iter-005/execution-board-follow-through.md) (paused), [execution-board](./iter-005/execution-board.md).

### Freeze notes

- TRACK-007 **paused** 2026-10-03. Do not resume until the human names it.
- iter-005 closed: [pm-retro-gate-pass](./iter-005/handoffs/2026-10-04-pm-retro-gate-pass.md).

## Forward

- After Milestone B: commit EP-088…091 (manipulation, connector, progressive paint, undo).
- After the forest: port tools one by one; each port writes the successor of its deprecated record first ([CHL-0033](./iter-006/challenges/CHL-0033-forest-product-records.md)).
- Infini sync returns only when the human reopens [CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md).
- Parked: REQ-15, REQ-08, CHL-0012, EP-035 measure, DeviceMap invert user interface, Mouse DragHandler, Infini undo apply (IN-038).
- Backlog: [backlog.md](./backlog.md)
