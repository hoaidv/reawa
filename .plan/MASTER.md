---
updated: 2026-08-13
current_iter: iter-003
owner: sm

# Campaign: Epaper owns the document — ink-box rework (supersedes Smart Group pilot lock).
execution:
  direction: vertical
  scope:
    modules: [epaper, infini]
    features:
      - epaper/device-document
      - epaper/ink-box
      - epaper/tool-modes
      - infini/tablet-sync
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
| Direction | **vertical** | The ink-box still needs both ends, but the *editing* end is now the tablet |
| Scope | `epaper` · device-document + ink-box + tool-modes **+** `infini` · tablet-sync | Re-locked 2026-08-13 (CHL-0008 adopted) |
| Stop line | **verified** | Design + implement through verify |
| Autonomy | **bounded** | Out-of-scope → backlog |

**Re-lock (2026-08-13, human directive — [CHL-0008](./iter-003/challenges/CHL-0008-architecture-rework.md) adopted).**
Document ownership inverts: **Epaper owns the working document in-session** (ink, ink-box
recognition, manipulation, undo); **Infini is viewer + navigator + persistence home**. Sync is
one-way per direction — Desktop→Tablet carries only an initial full-document load plus pan/zoom
viewport; Tablet→Desktop carries document changes. Recorded in
[epaper REQ-04…REQ-08](../.docs/modules/epaper/prd.md) and
[infini REQ-03](../.docs/modules/infini/prd.md#tablet-sync); rationale in the
[PM → architect handoff](./iter-003/handoffs/2026-08-13-pm-to-architect-device-document.md).

The previous lock (`infini/vector-document` + `epaper/tool-modes`, desktop as sole tree writer) is
superseded — its exit criteria failed human verify four times (CHL-0004…0007).

**Exit criteria:** epaper `[REQ-04]` device document, `[REQ-05]` on-device ink-box creation,
`[REQ-06]` on-device manipulation, and `[REQ-07]` one-way sync accepted by human (AC from the
thickened PRDs); design stories done for `[REQ-05]` + `[REQ-06]`. `[REQ-08]` node manipulation is
**thickened but not built** in this campaign.

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
| iter-003 | Smart Group pilot → **document ownership rework** | **active** — pilot failed verify; CHL-0008 adopted 2026-08-13, campaign re-locked | — | [iter](./iter-003/iter.md) |

## Now — iter-003

### Goal & capacity

- Goal (revised 2026-08-13): **Epaper owns the document.** On-device ink-box creation and
  manipulation with no peer round trip; one-way sync contract in both directions.
- Capacity: re-slice pending — the 11-story pilot plan is void (its stories assumed desktop
  authority). SM re-slices after the architect handoff.
- Risks: device now needs a document tree, hit-testing, and undo it never had; e-ink partial-refresh
  budget during live manipulation; desktop mirror must converge from a change stream it does not own.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **done** | — | [track](./tracks/TRACK-002-infini-vector-document.md) |
| TRACK-003 | planned | **paused** | Design complete (ADR-0014/0015 + `SRS-EP-07…14`) — `/sm` re-slices | [track](./tracks/TRACK-003-smart-group-pilot.md) |

### Open challenges / blocked

- **CHL-0008 resolved (adopted 2026-08-13)** — document ownership inverted to the device. Architect
  design landed the same day; TRACK-003 stays paused only until SM re-slices.
- CHL-0001…0003 adopted earlier; CHL-0004…0007 **superseded by CHL-0008**, retained as regression
  evidence (fixedInk resize, mode-correct preview, live direct manipulation, selection/enclose
  desync). Code hotfixes were discarded by the restore — do not treat as shipped.
- IN-006 remains cancelled (historical).

### Design packages in flight

- [epaper-tool-strip](./iter-003/design/epaper-tool-strip/) — STORY-EP-003 done; **needs revision**
  for real (non-ghost) selection affordances under epaper `[REQ-06]`
- [ink-box-ui](./iter-003/design/ink-box-ui/) — **deprecated** with `[SRS-IN-14]`; desktop-side
  ink-box authoring is out until multi-directional sync

### Execution board(s)

- [iter-003 execution-board](./iter-003/execution-board.md) — **PAUSED (CHL-0008)**, design complete
- [iter-002 board (final)](./iter-002/execution-board.md) — frozen

### Freeze notes

- **TRACK-003 paused** — see track freeze note. No `/dev` on verify-fix or CHL-0004…0007 paths;
  those device behaviours are re-specified from scratch under the new epaper REQs.
- Retro-gate: [pm-retro-gate-pass](./iter-002/handoffs/2026-08-11-pm-retro-gate-pass.md)
- iter-004 folder stays closed until the iter-003 retro gate passes.

## Forward

- **Done 2026-08-13:** `/architect` — [ADR-0014](../.docs/adr/ADR-0014-document-ownership-inversion.md)
  + [ADR-0015](../.docs/adr/ADR-0015-one-way-sync-contract.md), shared
  [domain doc](../.docs/domain/vector-document.md), `[SRS-EP-07]`…`[SRS-EP-14]`, device BDD, and the
  lifecycle propagation across both modules
  ([handoff](./iter-003/handoffs/2026-08-13-architect-to-sm-device-document.md))
- **Now:** `/sm` — re-slice TRACK-003 on the device SRS. Design story for `[SRS-EP-12]` and the
  ink-latency measurement come before any `[REQ-04]` implement story
- **Next campaign:** epaper `[REQ-08]` direct manipulation of any document node — thickened in
  [node-manipulation](../.docs/modules/epaper/features/node-manipulation/), designed and built in a
  distinct iteration
- Backlog: [backlog.md](./backlog.md)
