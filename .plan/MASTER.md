---
updated: 2026-09-05
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
      - epaper/erase
      - epaper/device-document
      - infini/infinity-canvas
      - infini/tablet-sync
      - infini/vector-document
  stop_line: verified
  autonomy: bounded
  out_of_scope: backlog
  wip: 2
  # Field follow-ups EP-070…072 ready. STORY-EP-069 done. Clipboard EP-044 done (human-verified 2026-09-04). Path B endpoint ink EP-047 done (human-verified 2026-09-05). STORY-EP-073 later.
  validated_by: ""
---

# Master Plan

Orientation: **history spine (thin)** → **Now (thick)** → **forward (thin)**.
Product truth in `.docs/`. Skill: [`execution-lock`](../.agent/personas/shared/execution-lock.md).

## Execution lock

| Field | Value | Why |
|---|---|---|
| Direction | **vertical** | One campaign: hand-on-paper wave end-to-end |
| Scope | epaper ink-box, tool-modes, connector-ink, region-sync, local-pen-ink, device-document, erase; infini canvas, tablet-sync, vector-document | REQ-10…14, 17, 18 + infini REQ-05 |
| Stop line | **verified** | design → BDD → implement → human confirm |
| Autonomy | **bounded** | Run inside lock; sink REQ-15 / REQ-08 |
| WIP | **2** | No implement story in flight. Field follow-ups [STORY-EP-070](./iter-005/stories/STORY-EP-070.md)…[STORY-EP-072](./iter-005/stories/STORY-EP-072.md) **ready**. [STORY-EP-069](./iter-005/stories/STORY-EP-069.md) ToolContextImpl/SelectionOverlay **done** (human-verified 2026-08-31). Erase EP-062…068 **done**. Clipboard [STORY-EP-044](./iter-005/stories/STORY-EP-044.md) **done** (human-verified 2026-09-04). Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **done** (human-verified 2026-09-05). [STORY-EP-073](./iter-005/stories/STORY-EP-073.md) later, not NOW. |
| Validated | — | Follow toggles EP-055 + IN-037 **done**; EP-038 + EP-039 **done**; **hand-touch human-approved** 2026-08-20 (20 mm / HT); IN-033 **done** (host); Device Settings on-device (REQ-20 / ADR-0031). Tool system [ADR-0033](../.docs/adr/ADR-0033-tool-abstraction.md) accepted. Inverse-op undo product + bind **done**; **human verified device undo/redo** 2026-08-27. Erase product [CHL-0028](./iter-005/challenges/CHL-0028-eraser-three-tools.md) **adopted**; [ADR-0034](../.docs/adr/ADR-0034-erase-clip-remnants.md) **accepted**; [ADR-0036](../.docs/adr/ADR-0036-toolcanvas-live-overlay.md) **accepted**. Erase implement **human-verified** 2026-08-31. [STORY-EP-069](./iter-005/stories/STORY-EP-069.md) / [ADR-0035](../.docs/adr/ADR-0035-tool-context-is-host-ports.md) **human-verified** 2026-08-31. Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **human-verified** 2026-09-05. |

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-16 | Table recognition REQ-15 | backlog — human excluded from TRACK-005 |
| 2026-08-16 | REQ-16 as separate id | retired → REQ-10 |
| 2026-08-13 | Generic any-node manipulation | REQ-08 parked |
| 2026-08-14 | Nested enclose / FREE_FORM | CHL-0011 **scheduled** 2026-09-05 via [CHL-0032](./iter-005/challenges/CHL-0032-nested-ink-box.md) (this lock, `epaper/ink-box`). CHL-0012 FREE_FORM still backlog |
| 2026-08-27 | DeviceMap invert user interface; Mouse DragHandler; further tool-system polish | backlog — TRACK-006 closed; do not continue unless a TRACK-005 story needs it |
| 2026-08-27 | Infini apply undo (`compound` / `set_ink_samples`); whole tablet→desktop undo sync | backlog — [STORY-IN-038](./iter-005/stories/STORY-IN-038.md) cancelled; waits independent sync algorithm |

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
- Capacity: committed stories include EP-053…073 / IN-036…038. [STORY-IN-033](./iter-005/stories/STORY-IN-033.md) **done**. [CHL-0028](./iter-005/challenges/CHL-0028-eraser-three-tools.md) **adopted**. Erase [STORY-EP-062](./iter-005/stories/STORY-EP-062.md)…[STORY-EP-068](./iter-005/stories/STORY-EP-068.md) **done**. [STORY-EP-069](./iter-005/stories/STORY-EP-069.md) **done** (human-verified 2026-08-31). Field follow-ups [STORY-EP-070](./iter-005/stories/STORY-EP-070.md)…[STORY-EP-072](./iter-005/stories/STORY-EP-072.md) **ready**. IN-038 **cancelled**. Clipboard [STORY-EP-044](./iter-005/stories/STORY-EP-044.md) **done** (human-verified 2026-09-04). Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **done** (human-verified 2026-09-05). [STORY-EP-073](./iter-005/stories/STORY-EP-073.md) later.
- Risks: [CHL-0022](./iter-005/challenges/CHL-0022-shipped-no-device-pan.md); [CHL-0027](./iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md); remaining Infini follow field test; device/Qt `epaper_bin` not built in this environment. Deprecated Infini snapshot rows — do not implement.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001…004 | planned | **done** | — | [tracks](./tracks/) |
| TRACK-005 | planned | **active** | Field follow-ups [STORY-EP-070](./iter-005/stories/STORY-EP-070.md)…[STORY-EP-072](./iter-005/stories/STORY-EP-072.md) **ready**. [STORY-EP-069](./iter-005/stories/STORY-EP-069.md) ToolContextImpl/SelectionOverlay **done**. Erase EP-062…068 **done**. Clipboard [STORY-EP-044](./iter-005/stories/STORY-EP-044.md) **done**. Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **done**. [STORY-EP-073](./iter-005/stories/STORY-EP-073.md) later. Remaining follow field test outstanding. | [track](./tracks/TRACK-005-hand-on-paper.md) |
| TRACK-006 | expedite | **done** | Closed 2026-08-27. Interrupted TRACK-005 (tool system / ADR-0033). Do not continue. | [track](./tracks/TRACK-006-tool-system-refactor.md) |

### Open challenges / blocked

- CHL-0012 / REQ-08 **not this lock**. [CHL-0011](../iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md) **scheduled** via [CHL-0032](./iter-005/challenges/CHL-0032-nested-ink-box.md).
- [CHL-0022](./iter-005/challenges/CHL-0022-shipped-no-device-pan.md) (Shipped “no device pan / no arrowheads” prose vs TRACK-005) — open; implement against new ids until Product Manager adopts.
- [CHL-0026](./iter-005/challenges/CHL-0026-inverse-op-undo.md) (Inverse-op undo, not whole-tree snapshots) — **adopted** 2026-08-27; ADR-0032 accepted; EP-059…061 **done** and **human-verified**; IN-038 **cancelled**.
- [CHL-0028](./iter-005/challenges/CHL-0028-eraser-three-tools.md) (Three exclusive erasers replace Path A / Path B) — **adopted** 2026-08-29; [prd-erase.md](../.docs/modules/epaper/prd-erase.md); [ADR-0034](../.docs/adr/ADR-0034-erase-clip-remnants.md); [ADR-0036](../.docs/adr/ADR-0036-toolcanvas-live-overlay.md); erase EP-062…068 **done** (human-verified 2026-08-31).
- [CHL-0027](./iter-005/challenges/CHL-0027-palm-travel-not-contact-count.md) (Palm rest by 20 mm travel, not 3-contact eat) — open; Product Manager triage; [SRS-EP-21](../.docs/modules/epaper/features/ink-box/srs-logic.md) still says ≥3 contacts.
- EP-032 parked in iter-004.

### Design packages in flight

- [hand-touch](./iter-005/design/hand-touch/) — EP-037 + EP-054 **done**; UI-EP-06 amended 2026-08-20 (20 mm + HT)
- [pen-button-map](./iter-005/design/pen-button-map/) — EP-056 **done** ([UI-EP-08](./iter-005/design/pen-button-map/ui-spec.md)); UI-IN-03 superseded
- [viewport-follow-epaper](./iter-005/design/viewport-follow-epaper/) — EP-053 **done** (UI-EP-07)
- [viewport-follow-infini](./iter-005/design/viewport-follow-infini/) — IN-036 **done** (UI-IN-04)
- Eraser glyphs (no package): `.docs/design/system/assets/icon-epaper-erase-*.svg` (2026-08-29)
- (frozen) connector-ends Path A toolbar — [STORY-EP-045](./iter-005/stories/STORY-EP-045.md) / [STORY-EP-046](./iter-005/stories/STORY-EP-046.md) blocked
- (queued) connector-attach, manual-create (clipboard chrome cancelled with EP-043)

### Execution board(s)

- [iter-005 execution-board](./iter-005/execution-board.md) — Field follow-ups EP-070…072 **ready**. [STORY-EP-069](./iter-005/stories/STORY-EP-069.md) **done**. Erase EP-062…068 **done**. Clipboard [STORY-EP-044](./iter-005/stories/STORY-EP-044.md) **done**. Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **done**. [STORY-EP-073](./iter-005/stories/STORY-EP-073.md) later.

### Freeze notes

- TRACK-004 **done**. Gate: [pm-retro-gate-pass](./iter-004/handoffs/2026-08-16-pm-retro-gate-pass.md).
- TRACK-005 field-test pause 2026-08-20: [sm-to-human-field-test](./iter-005/handoffs/2026-08-20-sm-to-human-field-test.md). Hand-touch join: [sm-to-human-hand-touch-verified](./iter-005/handoffs/2026-08-20-sm-to-human-hand-touch-verified.md). Inverse-op undo **adopted + bound** 2026-08-27 ([CHL-0026](./iter-005/challenges/CHL-0026-inverse-op-undo.md)). Device undo/redo **human-verified** 2026-08-27. Erase product **adopted** 2026-08-29 ([CHL-0028](./iter-005/challenges/CHL-0028-eraser-three-tools.md)); erase implement **human-verified** 2026-08-31. Clipboard [STORY-EP-044](./iter-005/stories/STORY-EP-044.md) **human-verified** 2026-09-04. Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **human-verified** 2026-09-05. [STORY-EP-073](./iter-005/stories/STORY-EP-073.md) later.
- TRACK-006 tool-system interrupt **closed** 2026-08-27: [sm-to-human-resume-track-005](./iter-005/handoffs/2026-08-27-sm-to-human-resume-track-005.md).

## Forward

- Inverse-op undo (device): [ADR-0032](../.docs/adr/ADR-0032-inverse-op-undo.md) **accepted**. [STORY-EP-059](./iter-005/stories/STORY-EP-059.md), [STORY-EP-060](./iter-005/stories/STORY-EP-060.md), and [STORY-EP-061](./iter-005/stories/STORY-EP-061.md) **done** and **human-verified** 2026-08-27. [STORY-IN-038](./iter-005/stories/STORY-IN-038.md) **cancelled** (tablet→desktop undo apply deferred until an independent sync algorithm).
- After remaining follow field test: still outstanding (does not block erase story review).
- Erase: [STORY-EP-062](./iter-005/stories/STORY-EP-062.md)…[STORY-EP-068](./iter-005/stories/STORY-EP-068.md) **done**. [STORY-EP-069](./iter-005/stories/STORY-EP-069.md) **done** (human-verified 2026-08-31). Field latency: [STORY-EP-070](./iter-005/stories/STORY-EP-070.md)…[STORY-EP-072](./iter-005/stories/STORY-EP-072.md) **ready**. Clipboard product [STORY-EP-044](./iter-005/stories/STORY-EP-044.md) **done**. Path B endpoint ink [STORY-EP-047](./iter-005/stories/STORY-EP-047.md) **done**. Do **not** start [STORY-EP-073](./iter-005/stories/STORY-EP-073.md) or Device Settings unless the human says so.
- Parked: REQ-15, REQ-08, CHL-0012, EP-035 measure, DeviceMap invert user interface, Mouse DragHandler, Infini undo apply (IN-038). Nested enclose **unparked** ([CHL-0032](./iter-005/challenges/CHL-0032-nested-ink-box.md)). Do **not** reopen TRACK-006.
- Backlog: [backlog.md](./backlog.md)
