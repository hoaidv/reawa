---
id: ADR-0014
title: Document ownership inversion — the device owns the working document
status: accepted
date: 2026-08-13
deciders: [architect, pm]
supersedes: null
amends: [ADR-0009, ADR-0011, ADR-0013]
source: CHL-0008
---

# ADR-0014 — Document ownership inversion

## Context

[ADR-0013](./ADR-0013-ink-box-tool-modes.md) §3 made Infini the **single writer of the tree**: the
device sent intent, the desktop recognized and applied, the device drew an advisory ghost and
discarded it when the authoritative `doc_snapshot` arrived. §4 spelled the consequence out plainly —
*"the ghost is advisory; authoritative geometry wins, even if it jumps."*

That jump is the product defect. Four fix waves attacked it and all four failed human verify:

| Challenge | Symptom | Real cause |
|---|---|---|
| [CHL-0004](../../.plan/iter-003/challenges/CHL-0004-fixedink-resize-boundary.md) | `fixedInk` resize detached boundary ink | Desktop transform semantics, invisible to the hand doing the drag |
| [CHL-0005](../../.plan/iter-003/challenges/CHL-0005-tablet-fixedink-resize-ghost.md) | Ghost previewed the wrong mode | A preview that is not the document can always be wrong |
| [CHL-0006](../../.plan/iter-003/challenges/CHL-0006-live-direct-resize.md) | Human asked to delete the ghost entirely | Direct manipulation is not previewable by proxy |
| [CHL-0007](../../.plan/iter-003/challenges/CHL-0007-selection-move-enclose-sync.md) | Selection residue, move snap-back, consecutive-enclose desync | Two authorities racing over one picture |

The ADR-0013 rationale for the split was sound at the time — it protected an epaper Non-Goal ("no
Smart Group recognition / geometry on-device") and avoided a second geometry implementation. PM has
since **withdrawn that Non-Goal** ([CHL-0008](../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md),
human directive) and re-specified the device as the editor:
[epaper REQ-04](../modules/epaper/prd.md#device-document)…[REQ-07](../modules/epaper/prd.md#one-way-sync).

The physics have not changed and they are the reason the split cannot work:

| Constraint | Consequence for a round trip |
|---|---|
| Panel refresh floor ≈250 ms (`kRefreshMinIntervalMs`) | A remote correction lands *after* the creator has seen and accepted their own result |
| Link is USB-Ethernet TCP, occasionally down | Any authority off-device makes editing conditional on the link |
| Ink is already local ([SRS-EP-01], ≤30 ms) | The creator's mental model is already "the device decides"; edits violated it |

## Decision

**The device owns the working document during a session. The desktop owns the file.**

### 1. Single writer, and it is the device

Epaper holds a document tree and is its **only writer** while a session is live. Recognition,
creation guards, reparenting, transforms, and undo all execute on the device. Infini writes nothing
into the document during a session.

This directly reverses ADR-0013 §3.

### 2. The panel paints the device's own document

No repaint is ever sourced from an inbound peer picture. `doc_snapshot`-as-truth is withdrawn; the
device's tree is the only thing it renders.

### 3. The desktop holds a mirror plus the file

Infini applies published device changes to a mirror and serializes that mirror to disk. The mirror
is a **follower**, not a peer authority. Desktop-side authoring is deprecated
([infini REQ-04](../modules/infini/prd.md#smart-group)) — not because it is hard, but because a
second writer is exactly what this ADR removes.

### 4. Exactly one document input reaches the device

The **initial full-document load**, at session start, reconnect, or explicit resync. It replaces the
local document wholesale, so it is legal only when the device has no unpublished changes. Everything
else about the sync contract is [ADR-0015](./ADR-0015-one-way-sync-contract.md).

### 5. Undo lives where editing lives

The snapshot ring of ADR-0013 §5 moves to the device unchanged in mechanism: push
`snapshotString()` before each structural op, depth 20, restore wholesale. The rationale is
untouched — the requirement is that one undo restores the previous tree *exactly*, and inverse-op
algebra gets reparent and coordinate-space changes wrong.

### 6. Node semantics are shared, not duplicated

There is now a second implementation of the tree (C++ on device, TypeScript on desktop). That is a
real cost and the honest one to pay; the mitigation is that **semantics live in one document** —
[`.docs/domain/vector-document.md`](../domain/vector-document.md) — with both implementations bound
to it, and shared fixtures (`features/vector-document/fixtures/ops/`) proving they agree.
[ADR-0010](./ADR-0010-tree-of-vectors.md) and [ADR-0011](./ADR-0011-smart-group.md) semantics are
unchanged; only their *host* moved.

### 7. Geometry constants stay world-units and shared

ADR-0013 §6's world-unit minimum enclose size (48 world units) survives verbatim, and for the same
reason: one rule at every zoom, on both peers. It now executes on the device.

## Consequences

- **Epaper stops being a thin client.** It needs a tree, hit-testing, transform math, an undo ring,
  and a change publisher. This is the largest single increase in device scope so far and the top
  risk in this ADR — see Risks.
- **Two geometry implementations must agree.** Mitigated by shared fixtures and the domain doc, not
  by hope. Any divergence is a `CHL-*`.
- **Editing survives a dead link.** The device is fully functional offline within a session; only
  publishing waits. This is a product gain that falls out of the decision rather than being built.
- **The desktop can no longer "fix" the device.** If the device's tree is wrong, the recovery is an
  explicit resync, not a silent correction. That is the intended trade: visible recovery over
  invisible divergence.
- **`pickables`, `tool_intent`, and `stroke_begin.intent` all become dead weight** — the device no
  longer asks anyone what it may touch. [SRS-IN-13] retires.
- **In-memory only this iter.** An app restart loses unpublished edits. Accepted by PM
  ([epaper Non-Goals](../modules/epaper/prd.md)); the mitigation is publish-per-op plus a visible
  pending-changes state, not persistence.
- **The multi-directional future is not blocked.** A single-writer session with an ordered change
  stream is a strict subset of what a CRDT or OT design would need; nothing here has to be undone to
  get there, only extended.

## Risks

| Risk | Severity | Mitigation |
|---|---|---|
| Device cannot hold + hit-test the document within the ≤30 ms ink budget | **High** — invalidates the rework | Measure before the first `[REQ-04]` story (PM handoff ask 5). If it fails, file a `CHL-*` rather than design around it |
| C++/TS geometry divergence | Medium | Shared fixtures, shared domain doc, parity scenarios in `srs-quality` |
| Live manipulation exceeds the partial-refresh budget | Medium | `[REQ-06]` bar: ≥5 Hz, 0 full-panel invalidations; CHL-0006 established that slow is acceptable |
| Unpublished work lost on restart | Low (accepted) | Publish per committed op; visible pending state |

## Amendments to prior ADRs

| Clause | Status after this ADR |
|---|---|
| [ADR-0009](./ADR-0009-shared-document-viewport.md) §1 document channel — *both peers apply the same append-only log* | **Amended** — one writer per session (the device); the desktop applies but does not author. The log becomes directional |
| ADR-0009 *same-picture rule* | **Kept, re-derived** — the device paints its own document ∩ viewport; the desktop converges by applying every published change. Same guarantee, opposite direction of flow |
| ADR-0009 conflict note ("Infini non-destructive viewer during draw") | **Promoted from a v0 caveat to the rule** |
| [ADR-0011](./ADR-0011-smart-group.md) §1–§6 node shape, roles, `inkScaleMode`, guards, membership | **Unchanged** — semantics survive intact |
| ADR-0011 placement of recognition on Infini | **Superseded** — recognition runs on the device |
| [ADR-0013](./ADR-0013-ink-box-tool-modes.md) §1 tool mode is device-local | **Kept** — and now trivially true, since the tool acts locally |
| ADR-0013 §2 `stroke_begin.intent` | **Superseded** — no intent crosses the wire; the device interprets its own stroke |
| ADR-0013 §3 Infini is the single writer | **Reversed** — the device is |
| ADR-0013 §4 `pickables` + `tool_intent` + advisory ghost | **Superseded** — the device hit-tests its own tree and manipulates real ink |
| ADR-0013 §5 snapshot undo, depth 20 | **Kept, re-homed** to the device |
| ADR-0013 §6 world-unit enclose guard, 48 units | **Kept** — now executed on the device |

## Alternatives Considered

| Approach | Gesture latency | Consistency | Cost | Notes |
|---|---|---|---|---|
| **Device owns the document; desktop mirrors + persists** | + local | + single writer | High device cost, duplicate geometry | **Chosen** |
| Keep desktop authority, make the ghost mode-correct | − round trip remains | + | Low | **Tried and failed** — CHL-0005; a correct ghost is still not the document |
| Keep desktop authority, optimistic apply on device with reconciliation | ~ | − reconciliation is where the jumps live | Medium | Rejected — this is the pilot with extra steps |
| CRDT / OT now, both peers write | + | + | − large, and unproven on the device | Deferred by PM; nothing here blocks it later |
| Device owns *and* persists (no desktop file authority) | + | + | − needs on-device storage, file UI, sync-on-restart | Deferred — PM Non-Goal this iter |
| Move manipulation to the device but leave recognition on the desktop | − split remains for create | − two authorities for one document | Medium | Rejected — CHL-0007's enclose desync is exactly this seam |
