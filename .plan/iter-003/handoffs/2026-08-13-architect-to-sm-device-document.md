---
from: architect
to: sm
date: 2026-08-13
iter: iter-003
---

# Hand-off: Architect → SM — the device owns the document

## Context

The [PM handoff](./2026-08-13-pm-to-architect-device-document.md) asked for two ADRs, a shared
domain doc, and an SRS decomposition. All are in. The design is complete enough to slice; nothing
below waits on further architecture.

### Decisions

| ADR | What it settles |
|---|---|
| [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) | **Who writes.** The device owns the working document in-session; the desktop owns the file. Reverses ADR-0013 §3, supersedes §2/§4, re-homes §5, keeps §1/§6 |
| [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) | **What crosses the wire.** Four message families, an ordered idempotent `doc_change` stream, and a handshake that makes a full load provably safe |

Two open questions from the PM handoff are answered in ADR-0015: change granularity is an **op
stream, one per committed gesture** (§2), and reconnect is a **handshake, not a reflex push** (§4).

### Shared semantics

[`.docs/domain/vector-document.md`](../../../.docs/domain/vector-document.md) +
[`domain/index.md`](../../../.docs/domain/index.md). Both peers now implement the tree, so its
anatomy stopped being an Infini SRS detail. Node kinds, roles, `inkScaleMode`, `layoutOffset`, and
op meanings live there; both `SRS-IN-04` and `SRS-EP-07` bind to it rather than restating it.

### New device SRS ids

| Id | File | Covers |
|---|---|---|
| `SRS-EP-07` | [device-document/srs-logic](../../../.docs/modules/epaper/features/device-document/srs-logic.md) | Device tree, stroke ingestion, op set, undo ring |
| `SRS-EP-08` | same file | One-way sync: inbound classification, load handshake, publish queue, preview |
| `SRS-EP-09` | [device-document/srs-data](../../../.docs/modules/epaper/features/device-document/srs-data.md) | Device-local structures, wire binding, sample retention, shared fixtures |
| `SRS-EP-10` | [ink-box/srs-logic](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) | Enclose recognition, guards, selection-create, draw-into membership |
| `SRS-EP-11` | same file | Hit-testing, selection, live move/resize, `inkScaleMode`, conformance contract |
| `SRS-EP-12` | [ink-box/srs-ui](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) | Selection overlay + manipulation chrome — **needs design** |
| `SRS-EP-13` | [device-document/srs-quality](../../../.docs/modules/epaper/features/device-document/srs-quality.md) | Ingestion, undo, sync invariants, offline parity, round-trip |
| `SRS-EP-14` | [ink-box/srs-quality](../../../.docs/modules/epaper/features/ink-box/srs-quality.md) | Create/manipulate budgets + CHL-0004…0007 regression bars |

Amended in place: `SRS-EP-02`, `SRS-EP-03`, `SRS-EP-04`, `SRS-EP-05`, `SRS-EP-06`, `SRS-IN-04`,
`SRS-IN-05`, `SRS-IN-06`, `SRS-IN-07`, `SRS-IN-08`, `SRS-IN-09`. Deprecated: `SRS-IN-10`, `-11`,
`-12`, `-14`, `-15`, `-16`. Retired: `SRS-IN-13`. All per the
[lifecycle map](../lifecycle-map-2026-08-13.md).

Architecture views: [epaper/architecture.md](../../../.docs/modules/epaper/architecture.md) is
**new** (the device was a thin client and never needed one);
[infini/architecture.md](../../../.docs/modules/infini/architecture.md) is revised to viewer +
mirror + persistence.

### BDD

Device specs authored under `epaper/features/*/bdd/`: `enclose-recognition`,
`draw-into-membership`, `selection-create-surround` (`@SRS-EP-10`), `smart-group-selection`
(`@SRS-EP-11`), `undo-ring` (`@SRS-EP-07`), `one-way-sync` (`@SRS-EP-08`, new). `device-tool-modes`
and `map-append-refresh` are revised to local-operation scenarios.

**Deviation from the lifecycle map, deliberate:** the infini originals were *not* deleted. They are
tagged `@deprecated` in place because they are the acceptance evidence for `done` stories
(STORY-IN-010, IN-014, IN-015, IN-016, IN-017), and `lifecycle.md` says retire in place. The epaper
files are rewrites, not copies.

## Asks

1. **Slice the design story first.** `SRS-EP-12` is the only `needs_design` surface and it blocks
   the manipulation implement stories. It also carries three questions that need hardware, not
   opinion: handle size in device units, the LOD cutoff value, and whether an undo affordance fits
   on a three-tool chip. Consider a spike inside the design story rather than after it.
2. **Slice the ink-latency measurement before any REQ-04 implement story.** This is the top risk in
   ADR-0014 and PM handoff ask 5. If a resident document plus hit-testing cannot hold pen-down →
   pixel at p95 ≤30 ms, the rework's premise fails and the right response is a `CHL-*`, not a
   workaround. Measuring it after the tree ships means discovering it too late to change direction.
3. **Order the device work as: document → recognition → manipulation → sync.** Recognition and
   manipulation both write the tree, so the tree is the real first slice. Sync can come last on the
   device because everything works offline by design — which is a scheduling gift worth using.
4. **Order the desktop work as: applier first.** `SRS-IN-07`'s inbound `doc_change` applier is the
   whole desktop change; the existing `rebuildWithRmInk` path writes flat primitives and nothing
   enters `VectorDocument`. Until the applier lands, the desktop cannot show or save device work.
5. **Keep the conformance rows in the same story as the manipulation work.** `SRS-EP-14`'s
   capability-descriptor rows (0 node-kind branches in the gesture router, reserved `rotation`
   field) are cheap to satisfy while writing the router and expensive to retrofit. A story that
   ships manipulation with a hard-coded SmartGroup branch passes its own tests and costs
   [REQ-08](../../../.docs/modules/epaper/prd.md#node-manipulation) its premise.
6. **Do not re-slice the desktop ink-box authoring stories.** `infini REQ-04` is deprecated; those
   behaviours come back only with multi-directional sync, which is a later campaign.

## Constraints

- **Ink latency outranks everything.** `SRS-EP-01`'s ≤30 ms is a floor no story may trade against.
- **No story may reintroduce a peer round trip inside an editing gesture** — that is the defect
  CHL-0004…0007 documented four times.
- **CHL-0004…0007 are regression criteria, not history.** Their failure modes appear as explicit
  "0" bars in `SRS-EP-14`; a story that ships without them is not done.
- Device constants inherited from the desktop (`TILE_LOD_SCALE = 0.35`, 8 CSS px tolerance) are
  **placeholders**, flagged open in `SRS-EP-12`. Do not let an implement story quietly adopt them.
- Shared fixtures are the only thing preventing two geometry implementations from drifting. A story
  that adds device geometry adds fixture coverage in the same slice.

## Out of scope

- On-device persistence, offline storage across restarts, multi-document.
- Multi-directional sync, CRDT/OT, conflict resolution.
- Desktop-side authoring of any kind this campaign.
- Rotation, multi-select, marquee, align/distribute, connector attachment to a box —
  [REQ-08](../../../.docs/modules/epaper/prd.md#node-manipulation), next campaign, `ADR-0016`
  deferred with it.

## Known gate state (expected, not a blocker)

| Check | Why it fails |
|---|---|
| Every active SRS has ≥1 story | `SRS-EP-09…14` are brand new — this is exactly what you are about to fix |
| Needs-design REQs have a design story | `epaper/REQ-05`, `REQ-06`, `REQ-08` — ask 1 |
| Design coverage: `epaper/ink-box` has no `kind:design` story | Same |
| No orphan active SRS | Code was restored to HEAD, so `@implements` tags are gone. Pre-existing; closes as stories land |
| `reawa/*` rows | Pre-existing, unrelated to this campaign |
