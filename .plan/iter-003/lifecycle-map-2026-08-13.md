---
title: Lifecycle propagation map — document ownership rework
date: 2026-08-13
owner: pm
source: CHL-0008
iter: iter-003
applies_to: [.docs/modules/epaper/**, .docs/modules/infini/**, .docs/adr/**]
---

# Lifecycle propagation map — CHL-0008 (Epaper owns the document)

PM decides **status**; the architect executes the SRS edits and sets `lifecycle` in the files.
Per [lifecycle.md](../../.agent/rules/lifecycle.md): **per element, no cascade** — a module stays
`active` while individual sections move. **Never delete an ID**; retire with `superseded-by`.

Rule of thumb for this rework: an SRS that describes *where a capability runs* changes; an SRS that
describes *what a capability means* survives.

## Legend

| Action | Meaning |
|---|---|
| **retire** | `lifecycle: retired` + `superseded-by`; the behaviour no longer exists anywhere |
| **deprecate** | `lifecycle: deprecated` + `superseded-by`; the behaviour moved to the device — the desktop section stays for history and for a possible return under multi-directional sync |
| **amend** | Same id, content revised, stays `active`; add a dated revision note |
| **new** | Architect allocates the next free id in that module |

---

## Infini — retire

| ID | File | Why |
|---|---|---|
| **[SRS-IN-13]** Tool intent transport | [tablet-sync/srs-logic.md](../../.docs/modules/infini/features/tablet-sync/srs-logic.md) | `stroke_begin.intent`, `doc_snapshot.pickables`, `tool_intent`, and the advisory-ghost/authority rule are exactly the round trip being deleted. Nothing inherits it — the device no longer sends intent because it acts. `superseded-by:` the new epaper document + sync SRS. |

Its BDD [tool-intent-transport.feature](../../.docs/modules/infini/features/tablet-sync/bdd/tool-intent-transport.feature)
retires with it (keep the file, mark the feature retired — never delete).
**Executed 2026-08-13:** file kept in place, tagged `@retired` at feature and scenario level.

## Infini — deprecate and re-home to Epaper

The **semantics** in these sections are correct and are inherited verbatim by the new epaper SRS.
Only their *home* is wrong.

| ID | File | Moves to (epaper) |
|---|---|---|
| **[SRS-IN-10]** Tool-armed enclose recognition | [vector-document/srs-logic.md](../../.docs/modules/infini/features/vector-document/srs-logic.md) | on-device recognition — [REQ-05](../../.docs/modules/epaper/prd.md#device-ink-box) |
| **[SRS-IN-11]** Selection, hit-test, move/resize, `inkScaleMode` | same | on-device manipulation — [REQ-06](../../.docs/modules/epaper/prd.md#device-manipulation) |
| **[SRS-IN-12]** Snapshot undo ring | same | on-device undo — [REQ-04](../../.docs/modules/epaper/prd.md#device-document). Undo belongs where editing happens |
| **[SRS-IN-15]** Draw-into membership | same | on-device — [REQ-05](../../.docs/modules/epaper/prd.md#device-ink-box) |
| **[SRS-IN-16]** Selection-create requires surround | same | on-device — [REQ-05](../../.docs/modules/epaper/prd.md#device-ink-box) |
| **[SRS-IN-14]** Ink-box ToolStrip + SelectionOverlay (desktop UI) | [vector-document/srs-ui.md](../../.docs/modules/infini/features/vector-document/srs-ui.md) | no desktop equivalent this campaign; device UI is [SRS-EP-05] + new. Design package [ink-box-ui](./design/ink-box-ui/) deprecates with it |

Their BDD files move to the epaper feature that inherits them, re-tagged to the new `@SRS-EP-*` ids.
Scenario text is reusable; the peer-round-trip steps are not.

**Executed 2026-08-13**, with one deliberate deviation from "move": the infini originals are
**kept in place and tagged `@deprecated`** rather than deleted, because they are the acceptance
evidence for `done` stories (STORY-IN-010, IN-014, IN-015, IN-016, IN-017) and
[lifecycle.md](../../.agent/rules/lifecycle.md) says retire in place, never delete. The epaper files
below are the **live** specs and are rewrites, not copies — the peer-round-trip steps are gone.

| Desktop original (now `@deprecated`) | Device spec (live) | Tag |
|---|---|---|
| `enclose-recognition.feature` | [epaper/ink-box/bdd](../../.docs/modules/epaper/features/ink-box/bdd/enclose-recognition.feature) | `@SRS-EP-10` |
| `draw-into-membership.feature` | [epaper/ink-box/bdd](../../.docs/modules/epaper/features/ink-box/bdd/draw-into-membership.feature) | `@SRS-EP-10` |
| `selection-create-surround.feature` | [epaper/ink-box/bdd](../../.docs/modules/epaper/features/ink-box/bdd/selection-create-surround.feature) | `@SRS-EP-10` |
| `smart-group-selection.feature` | [epaper/ink-box/bdd](../../.docs/modules/epaper/features/ink-box/bdd/smart-group-selection.feature) | `@SRS-EP-11` |
| `undo-ring.feature` | [epaper/device-document/bdd](../../.docs/modules/epaper/features/device-document/bdd/undo-ring.feature) | `@SRS-EP-07` |
| — (new) | [epaper/device-document/bdd/one-way-sync.feature](../../.docs/modules/epaper/features/device-document/bdd/one-way-sync.feature) | `@SRS-EP-08` |

**Also deprecate:** [infini REQ-04](../../.docs/modules/infini/prd.md#smart-group) — done in the PRD
(`superseded-by: [epaper REQ-05], [epaper REQ-06]`).

## Infini — amend (stay `active`)

| ID | File | Amendment |
|---|---|---|
| **[SRS-IN-04]** Tree model, node kinds, invariants, ops | [vector-document/srs-logic.md](../../.docs/modules/infini/features/vector-document/srs-logic.md) | Stays the semantic authority, but it now describes a model **both peers hold**. Architect: lift the shared anatomy into `.docs/domain/vector-document.md` and have this section link it rather than own it |
| **[SRS-IN-07]** Session roles, channels, wire | [tablet-sync/srs-logic.md](../../.docs/modules/infini/features/tablet-sync/srs-logic.md) | Downward: full-document load **only** at session start / reconnect / explicit resync — delete the "after Infini edits" and "orientation change" triggers. Upward: new document-change channel. Remove `pickables` |
| **[SRS-IN-08]** Sync quality / parity | [tablet-sync/srs-quality.md](../../.docs/modules/infini/features/tablet-sync/srs-quality.md) | Parity is measured as *mirror converges on published device changes*, not *device raster matches pushed snapshot*. Add the ≤300 ms mirror target and the 0-inbound-document-messages invariant |
| **[SRS-IN-09]** Persistence / transmit schemas | [vector-document/srs-data.md](../../.docs/modules/infini/features/vector-document/srs-data.md) | Add the document-change envelope (tablet → desktop) with ordering/idempotency; keep node + SmartGroup schemas unchanged |
| **[SRS-IN-06]** Vector document quality | [vector-document/srs-quality.md](../../.docs/modules/infini/features/vector-document/srs-quality.md) | Drop desktop Smart Group interaction scenarios (they move with SRS-IN-11/14); keep fidelity + round-trip; add mirror-replay idempotency |
| **[SRS-IN-05]** Document chrome | [vector-document/srs-ui.md](../../.docs/modules/infini/features/vector-document/srs-ui.md) | Unchanged in substance; note that the desktop is now viewer + persistence, so chrome is the *only* desktop document surface |

Feature indexes to update: [vector-document/index.md](../../.docs/modules/infini/features/vector-document/index.md)
(drop `REQ-04` from `parent_req`, mark moved ids) and
[tablet-sync/index.md](../../.docs/modules/infini/features/tablet-sync/index.md) (the one-line
summary still says "publish `viewport` + `doc_snapshot`").

## Epaper — amend

| ID | File | Amendment |
|---|---|---|
| **[SRS-EP-02]** Region sync logic | [region-sync/srs-logic.md](../../.docs/modules/epaper/features/region-sync/srs-logic.md) | The **local document** is authoritative for paint. Inbound: viewport always, full-document load only at start/reconnect/resync. Delete "replace local `m_vectorNodes` on every `doc_snapshot`". Outbound: document changes (plus preview samples) |
| **[SRS-EP-03]** Region sync quality | [region-sync/srs-quality.md](../../.docs/modules/epaper/features/region-sync/srs-quality.md) | Parity row is now device-document vs desktop-mirror, not device raster vs pushed snapshot. Keep map-before-refresh and settle budgets |
| **[SRS-EP-04]** Tool state and intent emission | [tool-modes/srs-logic.md](../../.docs/modules/epaper/features/tool-modes/srs-logic.md) | Heavy. Retire the **Enclose intent** and **Selection intent** sub-tables (no `intent` flag, no `pickables`, no `tool_intent`, no advisory ghost). Tools now invoke local document operations. Keep tool state, input routing, ToolChip exclusion rect, and the touch-unavailable fallback |
| **[SRS-EP-05]** ToolChip UI | [tool-modes/srs-ui.md](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) | Selection affordances are **real**, not a ghost corrected later. Add the session/publish status affordance required by [REQ-03](../../.docs/modules/epaper/prd.md#tool-modes) |
| **[SRS-EP-06]** Tool-mode quality | [tool-modes/srs-quality.md](../../.docs/modules/epaper/features/tool-modes/srs-quality.md) | Replace the enclose **round-trip** budget with the local budget (pen-up → box on panel p95 ≤500 ms, peer not involved) and add the offline-parity and commit-fidelity bars |
| **[SRS-EP-01]** Local pen ink | [local-pen-ink/srs-logic.md](../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) | **Unchanged.** Round 19 map and the ≤30 ms budget are the floor every new REQ must not regress |

BDD [device-tool-modes.feature](../../.docs/modules/epaper/features/tool-modes/bdd/device-tool-modes.feature)
needs its intent-emission scenarios rewritten as local-operation scenarios;
[touch-reachability-spike.feature](../../.docs/modules/epaper/features/tool-modes/bdd/touch-reachability-spike.feature)
and [map-append-refresh.feature](../../.docs/modules/epaper/features/region-sync/bdd/map-append-refresh.feature)
survive with the snapshot steps amended.

## Epaper — new (architect allocates ids from `SRS-EP-07`)

| Feature | Covers | Needs |
|---|---|---|
| [device-document](../../.docs/modules/epaper/features/device-document/index.md) | [REQ-04](../../.docs/modules/epaper/prd.md#device-document) + [REQ-07](../../.docs/modules/epaper/prd.md#one-way-sync) | `srs-logic` (tree, ingestion, undo, sync contract, change queue), `srs-data` (change envelope — or link SRS-IN-09), `srs-quality` |
| [ink-box](../../.docs/modules/epaper/features/ink-box/index.md) | [REQ-05](../../.docs/modules/epaper/prd.md#device-ink-box) + [REQ-06](../../.docs/modules/epaper/prd.md#device-manipulation) | `srs-logic` (recognition, guards, membership, selection, transforms), `srs-ui` (**needs design**), `srs-quality`, BDD inherited from the deprecated infini features |
| [node-manipulation](../../.docs/modules/epaper/features/node-manipulation/index.md) | [REQ-08](../../.docs/modules/epaper/prd.md#node-manipulation) — **next campaign** | `srs-product` + `srs-experience` authored now by PM; `srs-logic`/`srs-ui` deferred to that iter |

## ADRs

| ADR | Action |
|---|---|
| [ADR-0009](../../.docs/adr/ADR-0009-shared-document-viewport.md) shared document + viewport | **Amend** — the document channel is no longer a log both peers write. One writer in-session (device), one loader (desktop) |
| [ADR-0010](../../.docs/adr/ADR-0010-tree-of-vectors.md) tree of vectors | **Keep** — node kinds and invariants are unchanged; the tree just lives in two places |
| [ADR-0011](../../.docs/adr/ADR-0011-smart-group.md) Smart Group | **Keep semantics, supersede placement** — §"recognition runs on Infini" is wrong; roles, `inkScaleMode`, `layoutOffset`, guards, membership all stand |
| [ADR-0012](../../.docs/adr/ADR-0012-world-stroke-viewport-parity.md) stroke parity | **Keep** — unaffected |
| [ADR-0013](../../.docs/adr/ADR-0013-ink-box-tool-modes.md) ink-box tool modes | **Supersede §3 (Infini sole tree writer) and §4 (device pick → `tool_intent`)**; §1 device-local tool state and §6 min enclose size survive |
| **ADR-0014** (new) | Document ownership inversion — device owns the working document in-session |
| **ADR-0015** (new) | One-way sync contract v1 — message set, reconnect/resync, change ordering |
| **ADR-0016** (deferred) | Node manipulation model + capability descriptor — written in the [REQ-08](../../.docs/modules/epaper/prd.md#node-manipulation) iter |

## Stories

Per [lifecycle.md](../../.agent/rules/lifecycle.md) §"Plan follows product": stories referencing a
retired or deprecated SRS are **frozen, not deleted**. That is every implement story in
[TRACK-003](../tracks/TRACK-003-smart-group-pilot.md) except the local-ink and touch-spike work —
they assumed desktop authority. SM re-slices against the new epaper SRS after the architect handoff.

## Code

Code annotations may still point at deprecated ids until implementation catches up — that drift is
intentional and visible to `adlc audit`. Update `@implements` tags to the successor ids **in the
story that reimplements the behaviour**, not before.
