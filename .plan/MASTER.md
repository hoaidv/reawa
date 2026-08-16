---
updated: 2026-08-15
current_iter: iter-004
owner: sm

# Campaign: on-device connectors (REQ-09 + REQ-03 ToolChip). Through design → QA → implement.
execution:
  direction: vertical
  scope:
    modules: [epaper, infini]
    features:
      - epaper/connector-ink
      - epaper/tool-modes
      - epaper/ink-box
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
| Direction | **vertical** | One campaign end-to-end: design → BDD → implement → verify |
| Scope | `epaper/{connector-ink, tool-modes, ink-box}`, `infini/vector-document` | REQ-09 + the ToolChip split that arms it |
| Stop line | **verified** | `/designer`, `/qa`, `/dev` may all work; campaign ends on human confirm |
| Autonomy | **bounded** | Run inside the lock; stop at REQ-08 / CHL-0011 / CHL-0012 |
| WIP | **2** | IN-030 ∥ EP-033 NOW; EP-034 queued |

**Re-locked 2026-08-14 (human):** flip horizontal/`design-validated` → vertical/`verified`. Exit criteria: REQ-09 + REQ-03 ToolChip human-confirmed on device; Infini mirror 0 divergent connector nodes; EP-016/017 replay under ADR-0022. EXP-0002 guard corpus (≤2% FP) remains a **ship** gate, not a wave blocker. UI implement still `depends_on` done design stories (EP-026 → EP-028; EP-027 → EP-030).

**Out-of-scope log**

| Date | What came up | Sink |
|---|---|---|
| 2026-08-10 | Epaper on-device pan/zoom | backlog |
| 2026-08-10 | `reawa/*` | backlog |
| 2026-08-11 | STORY-IN-006 DocChrome | cancelled |
| 2026-08-11 | Rotation + connectors on a Smart Group | backlog → REQ-08 rotation; **connectors are now REQ-09** (this campaign) |
| 2026-08-11 | Mouse ink drawing on Infini (`Pen` on desktop) | backlog |
| 2026-08-11 | In-box content alignment / reflow | backlog |
| 2026-08-13 | Desktop-side ink-box authoring (Infini Selection / Ink-box tools) | backlog — infini `[REQ-04]` deprecated until multi-directional sync |
| 2026-08-13 | Multi-directional sync / modern doc-sync algorithm / CRDT | backlog — explicitly deferred by human |
| 2026-08-13 | On-device persistence, offline work, sync-any-moment | backlog — explicitly deferred by human |
| 2026-08-13 | Generic manipulation of any document node | epaper `[REQ-08]` — **not this lock** |
| 2026-08-14 | Nested enclose (CHL-0011), FREE_FORM / align-content (CHL-0012) | backlog — not this lock |
| 2026-08-14 | Guard-corpus false-positive bar (EXP-0002 Initiative 2) | **ship gate**, not lock gate — `/qa` may run in parallel with design |

## History spine

| Iter | Goal | Outcome | Carry-over | Links |
|---|---|---|---|---|
| iter-000 | Traceability backfill | closed; retro-gate passed | BDD optional | [iter](./iter-000/iter.md) |
| iter-001 | EXP-0001 + epaper promote | closed; S1 proven | → Infini REQs | [EXP-0001](./iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) |
| iter-002 | Infini + sync | **closed** — Must W4+W5 gated READY-WITH-CONCERNS | IN-010 → iter-003 | [iter](./iter-002/iter.md) · [retro](./iter-002/retro.md) |
| iter-003 | Epaper owns the document (REQ-04…07) | **closed** — W12 human confirm; campaign exited | REQ-08 / CHL-0011 / CHL-0012 stay parked | [iter](./iter-003/iter.md) · [retro](./iter-003/retro.md) |

## Now — iter-004

### Goal & capacity

- Goal: **On-device connectors** (REQ-09) + ToolChip 3 exclusive tools / 2 recognizer toggles / Undo+Redo (REQ-03). Default routing is auto-picked from ink → **Ink / Curve** (no picker at draw time).
- Capacity: 32 pts committed. W1 design 6 pts in flight; implement 26 pts gated by `depends_on` + BDD, not by the stop line.
- Risks: shipping default-on recognizers before the EXP-0002 guard corpus; regressing EP-016 enclose / EP-017 membership / EP-018–025 chrome.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **done** | — | [track](./tracks/TRACK-002-infini-vector-document.md) |
| TRACK-003 | planned | **done** | campaign exited 2026-08-14 | [track](./tracks/TRACK-003-smart-group-pilot.md) |
| TRACK-004 | planned | **active** | **IN-030 ∥ EP-033** · `/dev` | [track](./tracks/TRACK-004-on-device-connectors.md) |

### Open challenges / blocked

- CHL-0011 / CHL-0012 **future** — not in this lock.
- epaper `[REQ-08]` **future** — not in this lock.
- ADR-0019 / CHL-0018 amend **deferred**.
- Residue EP-007…011 / IN-020…026 remain **blocked** on iter-003.

### Design packages in flight

- [toolchip-recognizers](./iter-004/design/toolchip-recognizers/) — STORY-EP-026
- [connector-chrome](./iter-004/design/connector-chrome/) — STORY-EP-027

### Execution board(s)

- [iter-004 execution-board](./iter-004/execution-board.md) — **NOW** `/dev` IN-030 ∥ EP-033
- [iter-003 execution-board](./iter-003/execution-board.md) — **frozen** (campaign closed)

### Freeze notes

- TRACK-003 **done**. Retro: [iter-003/retro.md](./iter-003/retro.md). Gate: [pm-retro-gate-pass](./iter-003/handoffs/2026-08-14-pm-retro-gate-pass.md).

## Forward

- **Now:** `/dev` **IN-030 ∥ EP-033**. EP-031 human-verified. EP-034 queued.
- Parked (not committed): epaper `[REQ-08]`, nested enclose (CHL-0011), FREE_FORM / align-content (CHL-0012).
- **Next wave (iter-005, not opened):** [BS-0002](./iter-004/brainstorms/BS-0002-iter-005-feature-wave.md) — natural pen-on-paper (erase, copy/cut/paste, connector ends + mid-attachments); finger pan/zoom and manual frame/primitives parked pending Non-Goal reversal; AI unspecified. **Do not open iter-005 until retro-gate.**
- Backlog: [backlog.md](./backlog.md)
