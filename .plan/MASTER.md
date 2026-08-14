---
updated: 2026-08-14
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
- Capacity: W8–W12 re-sliced (45 pts vs capacity 15 — same stretch as the pilot). Only W8 is
  NOW (EP-012 design ∥ EP-013 latency). The 11-story pilot plan is void.
- Risks: device now needs a document tree, hit-testing, and undo it never had; e-ink partial-refresh
  budget during live manipulation; desktop mirror must converge from a change stream it does not own.

### Tracks

| Track | Kind | Status | Cursor (next) | Link |
|---|---|---|---|---|
| TRACK-001 | planned | **done** | — | [track](./tracks/TRACK-001-infini-infinity-canvas.md) |
| TRACK-002 | planned | **done** | — | [track](./tracks/TRACK-002-infini-vector-document.md) |
| TRACK-003 | planned | **active** | W12 done → `/pm` + human verify | [track](./tracks/TRACK-003-smart-group-pilot.md) |

### Open challenges / blocked

- **CHL-0008 resolved (adopted 2026-08-13)** — document ownership inverted to the device. Architect
  design landed the same day; SM re-sliced 2026-08-13.
- **CHL-0009 resolved (adopted 2026-08-13)** — `device-document/srs-logic.md` landed
  (`[SRS-EP-07]` / `[SRS-EP-08]`). IN-027 **done**. EP-014 **done**. EP-015 **done**.
  EP-020 **done** (W12). Verdict: READY-WITH-CONCERNS (op-type aliases — use SRS-IN-09 transmit names).
- **CHL-0010 superseded-in-part (2026-08-14)** — undo **chrome** adopted as actions after a gap
  ([CHL-0016](./iter-003/challenges/CHL-0016-undo-redo-toolbar.md) / [ADR-0018](../.docs/adr/ADR-0018-undo-redo-chip-actions.md)).
  Selection-create invocation remains Enclose CTA (CHL-0013), not a tool.
- **CHL-0011 adopted → future (2026-08-13)** — nested enclose (capture Smart Groups as content).
  Not W10–W12. Current enclose stays free-ink only; Enclose CTA refuses if SmartGroup selected.
- **CHL-0012 adopted → future (2026-08-13)** — ink-box sizing `FREE_FORM` / `WRAP_CONTENT` and
  `align-content` (TOP|RIGHT|BOTTOM|LEFT). Not W10–W12. Shipping stays `inkScaleMode` +
  non-expanding membership.
- **CHL-0013 adopted (2026-08-13)** — rubber-band + 6 anchors + Enclose CTA. Design
  [STORY-EP-022](./iter-003/stories/STORY-EP-022.md) **ready** → `/designer`.
- CHL-0001…0003 adopted earlier; CHL-0004…0007 **superseded by CHL-0008**, retained as regression
  evidence (fixedInk resize, mode-correct preview, live direct manipulation, selection/enclose
  desync). Code hotfixes were discarded by the restore — do not treat as shipped.
- IN-006 remains cancelled (historical).

### Design packages in flight

- [device-selection-chrome](./iter-003/design/device-selection-chrome/) — STORY-EP-012 **done**
  `[UI-EP-02]`. Do not port the EP-003 ghost or deprecated ink-box-ui.
- [epaper-tool-strip](./iter-003/design/epaper-tool-strip/) — STORY-EP-003 done; ToolChip is composed
  into EP-012, not redesigned. Ghost selection scenes are withdrawn.
- [ink-box-ui](./iter-003/design/ink-box-ui/) — **deprecated** with `[SRS-IN-14]`; desktop-side
  ink-box authoring is out until multi-directional sync

### Execution board(s)

- [iter-003 execution-board](./iter-003/execution-board.md) — **W12 done** → `/pm` + human campaign verify
- [iter-002 board (final)](./iter-002/execution-board.md) — frozen

### Freeze notes

- **TRACK-003 active (W9)** — freeze note is history. No `/dev` on verify-fix or CHL-0004…0007
  paths. Residue EP-007…011 / IN-020…026 stays blocked. EP-013 latency gate **passed**.
- Retro-gate: [pm-retro-gate-pass](./iter-002/handoffs/2026-08-11-pm-retro-gate-pass.md)
- iter-004 folder stays closed until the iter-003 retro gate passes.

## Forward

- **Done 2026-08-13:** `/architect` design + `/sm` re-slice (EP-012…020, IN-027…028)
- **Now:** **W12 done.** [STORY-EP-020](./iter-003/stories/STORY-EP-020.md) ∥
  [STORY-IN-028](./iter-003/stories/STORY-IN-028.md) **done** (QA + human confirm 2026-08-14).
  Committed campaign stories are complete. Next: **`/pm`** gate-close, then human verify
  REQ-04…07 (stop_line `verified`). ADR-0019/CHL-0018 amend stays deferred.
- **Next campaign:** epaper `[REQ-08]` direct manipulation of any document node — thickened in
  [node-manipulation](../.docs/modules/epaper/features/node-manipulation/), designed and built in a
  distinct iteration. Also: nested enclose (CHL-0011); ink-box sizing / align-content (CHL-0012);
  selection-create chrome (CHL-0013) if adopted into this track.
- Backlog: [backlog.md](./backlog.md)
