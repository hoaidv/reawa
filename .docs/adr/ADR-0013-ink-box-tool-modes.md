---
id: ADR-0013
title: Ink-box tool modes, intent transport, and undo
status: accepted
date: 2026-08-11
deciders: [architect, pm]
supersedes: null
amends: ADR-0011
---

# ADR-0013 — Ink-box tool modes, intent transport, and undo

## Context

[ADR-0011](./ADR-0011-smart-group.md) defined the SmartGroup node and named two creation paths,
with enclose recognition running as **propose → user accepts**. PM has since adopted a different
interaction model ([REQ-04](../modules/infini/prd.md#smart-group),
[epaper REQ-03](../modules/epaper/prd.md#tool-modes)):

- A Smart Group is **never** created unprompted. The creator arms an `Ink-box` tool, or issues an
  explicit command on a selection. Arming the tool **is** the confirmation, so the propose/accept
  step in ADR-0011 §4A has no user left to serve.
- The tablet gains a three-tool toolbar (`Selection · Pen · Ink-box`), which makes Epaper an
  input-mode participant for the first time.

The constraints that shape the decision are all in shipped code:

| Constraint | Where |
|---|---|
| Epaper holds no document tree and does no geometry recognition | [epaper Non-Goals](../modules/epaper/prd.md) |
| Production wire is `viewport` + `doc_snapshot` + `stroke_*`; `doc_op` is deferred | [SRS-IN-07](../modules/infini/features/tablet-sync/srs-logic.md) |
| Panel refresh floor ≈250 ms, ghosting allowed | `epaper/tabletcanvasitem.cpp` `kRefreshMinIntervalMs` |
| Individual picking is off below 0.35 viewport scale | `infini/src/canvas/TileCache.ts` |
| No undo stack exists anywhere in `infini/src` | code scan 2026-08-11 |

## Decision

### 1. Tool mode is device-local UI state

Tool selection lives in each peer's UI layer. It is **not** a document property, **not** a session
field, and **not** synced. Infini neither reads nor drives Epaper's tool, and vice versa.

Rationale: a synced mode needs reconciliation rules for simultaneous changes, and on a panel whose
refresh trails by ≥250 ms a *remote* mode change is unattributable — the creator would see the tool
move under them with no cause. Locality removes both problems and costs nothing, because the only
thing the peer needs is the **intent of a specific action**, not the mode that produced it.

### 2. Intent rides on the stroke that carries it

`stroke_begin` gains one optional field:

```text
{ "type": "stroke_begin", "id": …, "brush": {…}, "intent": "ink" | "enclose" }
```

Absent or unrecognised → `ink` (today's behaviour). The stroke still streams as ordinary samples
and still paints locally as ink; `intent` only tells Infini how to interpret the **completed**
stroke.

Rationale: the stroke is already the unit the creator acts with, so intent attached to it can never
desynchronise from the mode that produced it — unlike a separate "set mode" message, which can
arrive out of order relative to the stroke it was meant to modify. The field is additive, and both
peers already ignore unknown keys.

### 3. Infini is the single writer of the tree

Recognition, the creation guards, reparenting, transforms, and undo all execute on Infini.
Epaper contributes tool intent, pen samples, and pick/drag intent — nothing else. Epaper never
fits a rectangle, never computes containment, and never holds a `VectorDocument`.

Rationale: preserves the Epaper Non-Goal, avoids a second geometry implementation in C++ that would
have to stay bit-compatible with the TypeScript one, and keeps one authority for conflict-free ops.

### 4. Epaper `Selection` picks locally against a published pickable list

`doc_snapshot` gains a sibling array to `nodes`:

```text
{ "type": "doc_snapshot", "nodes": [ … ], "pickables": [ { "id", "kind", "bounds" } ] }
```

In the pilot `kind` is `smart_group` only. Epaper hit-tests locally (it already has
`panelToWorld` and the drawing region), draws the drag as a **local ghost**, and emits intent:

```text
{ "type": "tool_intent", "action": "select" | "move" | "resize", "nodeId": …, … }
```

Infini applies the op and returns an authoritative `doc_snapshot`; the device discards its ghost
on arrival.

Rationale: a local hit-test keeps selection feedback off the network, which matters because the
round trip plus refresh floor would otherwise make a pick feel broken. A narrow `tool_intent`
channel — rather than opening bidirectional `doc_op` — keeps the pilot decoupled from the deferred
op-log migration, which is explicitly backlogged. `tool_intent` retires when that migration lands.

### 5. Undo is snapshot-based and bounded

Before every structural op, Infini pushes `VectorDocument.snapshotString()` onto a ring buffer
(pilot depth **20**). Undo restores the previous snapshot wholesale.

Rationale: REQ-04 requires that one undo restore the previous tree *exactly*, and a snapshot gives
that by construction. Inverse-op algebra would be more code and more failure modes (reparent and
coordinate-space changes are the exact cases it gets wrong) for a pilot whose documents are small.
The memory cost is bounded and measured in srs-quality.

### 6. The enclose size guard is in world units

The minimum fitted-rect size is a **world-unit** constant, not screen pixels and not a user
setting. Pilot default: **48 world units** on the shorter side.

Rationale: a screen-px threshold would make the same gesture succeed or fail depending on zoom, and
would behave differently on the panel than on the desktop. World units give one rule for both peers
at every zoom. It is a tuning constant, not a preference — the tuning signal is the ≥80% first-try
metric in the PRD.

## Consequences

- Epaper stays a thin client: toolbar, intent, local ghost. No tree, no recognizer, no ops.
- Epaper must reach the capacitive touch layer from Qt — **unverified**, and the top risk in this
  ADR. If touch is unreachable the toolbar needs a different actuation (pen-on-strip or hardware
  button) and epaper REQ-03's design changes.
- Two intent paths coexist (`stroke_begin.intent` for creation, `tool_intent` for manipulation)
  until the ADR-0009 op-log migration replaces both.
- `doc_snapshot` grows; Epaper must tolerate the new array and Infini must keep sending `nodes`
  unchanged for older devices.
- Undo covers structural ops only in the pilot; viewport and tool changes are not undoable.
- ADR-0011 §4A's propose/accept step is **withdrawn** (see below); §5's "best-effort with undo"
  survives, now measured against the PRD first-try metric.

## Amendment to ADR-0011

| ADR-0011 clause | Status after this ADR |
|---|---|
| §4A enclose recognition *proposes* a SmartGroup; user accepts | **Withdrawn** — arming `Ink-box` is the confirmation; creation is immediate and undoable |
| §4A bounds/boundary/content handling on accept | Unchanged — applies at creation time |
| §4B explicit multi-select create | **Amended** — requires a surround stroke among the selection (artificial close for test); refuse if none (ADR-0011 §4B) |
| §5 best-effort recognition, false positives undoable | Unchanged; guards added (world-unit min size, ≥1 ink) |
| §6 ops list incl. `recognize_enclose` | `recognize_enclose` becomes an **internal** Infini step, not a wire op |
| §1–§3 node shape, local transform, `inkScaleMode` | Unchanged |

## Alternatives Considered

| Approach | Mode safety | Device cost | Wire cost | Notes |
|---|---|---|---|---|
| **Local mode + intent on stroke + narrow `tool_intent`** | + | low | +1 field, +1 message | **Chosen** |
| Synced tool mode as a session field | − races, − e-ink staleness | low | new channel + reconciliation | Rejected — remote mode change is unattributable on the panel |
| Separate "set mode" message, strokes unmarked | − ordering races | low | new message | Rejected — mode and stroke can desynchronise |
| Recognition on device | + | **high** — tree + geometry in C++ | none | Rejected — epaper Non-Goal; duplicate geometry authority |
| Open bidirectional `doc_op` now | + | high | large | Rejected — drags in the deferred op-log migration for a pilot |
| Relay every pick to Infini (no local hit-test) | + | none | chatty | Rejected — round trip + 250 ms refresh makes selection feel broken |
| Inverse-op undo | + | — | — | Rejected for pilot — reparent/coordinate-space inverses are the failure-prone cases |
| Screen-px size guard, user-settable | − | — | — | Rejected — zoom-dependent, device-dependent, pushes an unsolved question onto the user |
