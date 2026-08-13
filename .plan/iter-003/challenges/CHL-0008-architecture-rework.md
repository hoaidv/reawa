---
id: CHL-0008
title: Total architecture rework — stop patching Smart Group sync/selection
status: resolved
resolution: adopted
severity: high
raised_by: sm
resolved_by: pm
iter: iter-003
date: 2026-08-11
resolved: 2026-08-13
related: [REQ-03, REQ-04, ADR-0011, ADR-0013, SRS-EP-04, SRS-IN-13, CHL-0004, CHL-0005, CHL-0006, CHL-0007, TRACK-003]
expedite: true
interrupts_track: TRACK-003
---

# CHL-0008 — Architecture rework (human directive)

## Conflict

Human **restored implementation to latest git commit** and directed: **total re-work on the architecture** — not another verify-fix / CHL-0004…0007 patch wave on the current design.

Working tree for `Epaper/` + `infini/` matches `HEAD` (`63c3659` Improve SmartGroup). Uncommitted plan artifacts (CHL-0004…0007, late stories EP-008…011 / IN-023…026, handoffs) remain on disk as history only until PM decides.

## Why SM cannot absorb this

| Layer | Owner |
|---|---|
| Product trade-off (keep pilot UX vs redesign sync/selection model) | **PM** |
| New / superseding SRS + ADR | **Architect** |
| Story re-slice after adopted design | **SM** |
| Code | **Dev** (after stories) |

Continuing EP-006 ∥ IN-019 / CHL-0007 hotfixes would fight the restore and the rework directive.

## Ask PM

1. **Adopt / Defer / Reject** this challenge.
2. If **Adopt**: re-lock `execution:` (scope, stop line) — e.g. pause “verified” on current pilot and open an architecture redesign campaign; name what stays (REQ-03/04 UX?) vs what is in play (device ghost vs mutate, snapshot authority, enclose sync).
3. Route **`/architect`** for a green-field (or surgically superseding) design before SM slices implement stories.
4. Confirm disposition of CHL-0004…0007 (superseded by rework vs keep as constraints).

## Freeze (SM)

TRACK-003 paused — see track freeze note. No new implement stories until PM resolution + architect handoff.

## Resolution

**Adopted** 2026-08-13 by PM (human directive, 2026-08-13).

### Root cause accepted

The pilot's fragility is **not** a bug stack — it is the ownership model. Every manipulation had to
travel Pen → Tablet → Desktop → recognize/apply → Tablet, so the creator's own gesture was corrected
by a peer round trip. CHL-0004…0007 were four attempts to make that round trip feel local; none can,
because the device never held the truth.

### Decision — invert document ownership

1. **Epaper owns the working document in-session.** The device holds the document tree, ingests its
   own ink, recognizes and creates ink-boxes locally, and manipulates them locally. No peer round
   trip in any editing gesture.
2. **Infini becomes viewer + navigator + persistence home.** It opens/saves the file, holds a
   mirror, and drives pan/zoom. Its own Ink-box / Selection authoring tools are **deprecated** this
   iter and return when multi-directional sync lands.
3. **Sync becomes one-way per direction.**
   - Desktop → Tablet: **initial full-document load** (connect / reconnect / explicit resync) and
     **pan/zoom viewport events**. Nothing else. No post-edit `doc_snapshot`, no `pickables`.
   - Tablet → Desktop: **document changes**.
4. **Deferred by name (not this iter):** modern document-synchronization algorithm, multi-directional
   sync, on-device persistence with offline work and sync-any-moment.

### What stays

- Epaper [REQ-03](../../../.docs/modules/epaper/prd.md#tool-modes) three-tool toolbar
  (`Selection · Pen · Ink-box`), finger-switched, device-local tool state.
- SmartGroup node semantics from [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md): boundary vs
  content roles, `inkScaleMode`, per-ink `layoutOffset` UV, enclose guards, draw-into membership.
- World-width stroke parity ([ADR-0012](../../../.docs/adr/ADR-0012-world-stroke-viewport-parity.md))
  and Round 19 local ink ([SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md)).

### What is in play

Ownership, write authority, sync direction, where recognition runs, where undo lives, where selection
and manipulation are specified, and the desktop's editing role.

### Re-lock

Execution scope moves to epaper editing features + the infini sync contract; see
[MASTER.md](../../MASTER.md). TRACK-003 stays paused; SM re-slices only after `/architect` delivers
the superseding ADRs and SRS.

### Disposition of CHL-0004…0007

**Superseded by this rework** — their code hotfixes were discarded by the restore and are not
shipped. They are retained as **evidence of failure modes the new design must not reproduce**:

- [CHL-0004](./CHL-0004-fixedink-resize-boundary.md) — `fixedInk` resize must scale-and-translate,
  never mutate `bounds` alone, or boundary ink detaches from the frame.
- [CHL-0005](./CHL-0005-tablet-fixedink-resize-ghost.md) — a preview that is not mode-correct is
  worse than no preview.
- [CHL-0006](./CHL-0006-live-direct-resize.md) — the creator wants **live direct manipulation** of
  real ink, not an advisory outline. Slow e-ink refresh is acceptable; a wrong picture is not.
- [CHL-0007](./CHL-0007-selection-move-enclose-sync.md) — selection residue, move snap-back, and
  consecutive-enclose desync were all peer-authority artifacts.

The new design must pass these four as regression cases without a peer round trip.

### Ask 4 — plan residue

Late stories EP-008…011 and IN-023…026 stay on disk as history. They are **not** scheduled; the
device-side behaviours they chased are re-specified from scratch under the new epaper REQs.

## Product doc updates

- [.docs/modules/epaper/prd.md](../../../.docs/modules/epaper/prd.md) — new `[REQ-04]`…`[REQ-08]`;
  `[REQ-02]` + `[REQ-03]` amended; Non-Goals and Success Metrics rewritten.
- [.docs/modules/infini/prd.md](../../../.docs/modules/infini/prd.md) — `[REQ-03]` amended to the
  one-way contract; `[REQ-04]` `lifecycle: deprecated` with `superseded-by`; `[REQ-02]` re-scoped.
- Lifecycle propagation map (per element, no cascade):
  [lifecycle-map-2026-08-13.md](../lifecycle-map-2026-08-13.md).
- New epaper features: `device-document/`, `ink-box/`, `node-manipulation/`.

### Architect execution (2026-08-13)

- Decisions: [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) ownership
  inversion (amends ADR-0009, ADR-0011, ADR-0013) ·
  [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) one-way sync contract v1.
  `ADR-0016` (node manipulation model) deferred to the `[REQ-08]` campaign.
- Shared semantics: [domain/vector-document](../../../.docs/domain/vector-document.md) +
  [domain/index](../../../.docs/domain/index.md) — both peers now implement the tree.
- New: `[SRS-EP-07]` device document + undo · `[SRS-EP-08]` one-way sync · `[SRS-EP-09]` device data
  · `[SRS-EP-10]` recognition + membership · `[SRS-EP-11]` manipulation · `[SRS-EP-12]` selection
  chrome (**needs design**) · `[SRS-EP-13]` / `[SRS-EP-14]` quality.
- Retired: `[SRS-IN-13]`. Deprecated: `[SRS-IN-10]`, `[SRS-IN-11]`, `[SRS-IN-12]`, `[SRS-IN-14]`,
  `[SRS-IN-15]`, `[SRS-IN-16]`. Amended: `[SRS-EP-02]`…`[SRS-EP-06]`, `[SRS-IN-04]`…`[SRS-IN-09]`.
- BDD: device features authored under `epaper/features/*/bdd/`; the infini originals tagged
  `@deprecated` in place (they are acceptance evidence for `done` stories).
- Architecture views: [epaper/architecture.md](../../../.docs/modules/epaper/architecture.md) (new)
  and [infini/architecture.md](../../../.docs/modules/infini/architecture.md) (revised).
- Handoff: [architect → SM](../handoffs/2026-08-13-architect-to-sm-device-document.md).

## Interrupt / expedite (when applicable)

TRACK-003 remains paused for **slicing only**: the architect work is complete. Resume path:
`/sm` re-slice → `/designer` (`[SRS-EP-12]`) → `/qa` → `/dev`.
