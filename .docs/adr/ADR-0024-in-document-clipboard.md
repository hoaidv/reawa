---
id: ADR-0024
title: In-document clipboard ops (one slot)
status: proposed
date: 2026-08-19
deciders: [architect, pm]
supersedes: null
source: TRACK-005 / [REQ-12]
---

# ADR-0024 — In-document clipboard ops (one slot)

## Context

[Epaper REQ-12](../modules/epaper/prd.md#clipboard) requires copy / cut / paste **on the tablet**, in-document only: one slot, paste offset so the copy is visible, cut = copy + delete, paste one undoable op, OS / cross-app paste out. The device already has a snapshot undo ring ([SRS-EP-07](../modules/epaper/features/device-document/srs-logic.md), [ADR-0014](./ADR-0014-document-ownership-inversion.md) §5) and an ordered `doc_change` stream ([ADR-0015](./ADR-0015-one-way-sync-contract.md)).

Quality goals: local sufficiency (works with the link down), undo exactness (±1 px @ 100% zoom), publish the resulting ops when linked (REQ-07).

## Decision

v1 clipboard is a **device-local slot**, not an OS pasteboard and not a document node.

| Rule | Value |
|---|---|
| Cardinality | **One** slot. A later copy/cut replaces it |
| Contents | Deep-cloned selected subtree(s): nodes + child ink, with **new ids** minted only at **paste** |
| Lifetime | Session memory. App restart → empty slot. Never in `doc_load` / SVG |
| Copy | Slot ← clone of selection; document unchanged; **0** undo entries |
| Cut | Slot ← clone; then **one** `remove` (or equivalent snapshot) of the selection — **one** undo restores the originals |
| Paste | Insert clones at `sourceAABB.min + (24 u, 24 u)` world; mint new ids; **one** undo removes the copies. Empty slot → no-op, **0** undo |
| Offset | Documented constant **+24 world units** in +x and +y from the **copied** AABB min. If the offset would land fully outside the current `drawingRegion`, clamp the AABB into the region (keep size) so the paste is visible |
| Selection after paste | The **new** nodes are selected |
| Publish | Copy publishes **0** ops. Cut publishes the remove. Paste publishes the insert(s) (one `doc_change` per committed gesture; a multi-node paste is still **one** gesture → one snapshot undo, one or more ops only if the wire grammar already requires one op per node — see below) |
| Wire | Prefer **one** `restore_snapshot` after paste if the insert set is multi-node and no `insert_subtree` op exists yet; otherwise a closed `duplicate_subtree` op. v1 **locks `duplicate_subtree`** (below) so Infini does not have to infer a snapshot |
| Cross-app / OS | Out. Slot is invisible to macOS pasteboard |

### `duplicate_subtree` (new document op)

```text
{ type: "duplicate_subtree", payload: { copies: [{ newId, fromId, parentId, dxy: {x: 24, y: 24} }, …] } }
```

`fromId` nodes must exist in the **pre-paste** tree (cut case: they exist in the slot, not the tree — paste after cut uses slot bytes, so the op payload carries the **cloned node bodies**, not `fromId` into a missing tree):

```text
{ type: "duplicate_subtree", payload: { nodes: [Node, …], dxy: {x: 24, y: 24} } }
```

v1 uses the **embedded-nodes** form always (works after cut and with the link down). Infini applies by inserting those nodes with the given ids (already new). Device mints ids at paste time.

Cut remains `remove` of original ids (existing op). Copy is local-only.

## Consequences

- Slot anatomy lives in [SRS-EP-09](../modules/epaper/features/device-document/srs-data.md) (device-local). Behaviour: [SRS-EP-31](../modules/epaper/features/device-document/srs-logic.md).
- Undo stack after cut+paste: undo paste → copies gone, originals still gone, slot still full; undo again → originals restored. Matches REQ-12 acceptance.
- Does not fork OS clipboard; Designer must not invent a “paste from Mac” control.
- Sensitivity: **one undo vs one op per node**. Snapshot-per-gesture already exists; `duplicate_subtree` keeps the mirror an op-stream without a second full snapshot on the wire for a 3-node paste.

## Alternatives Considered

| Approach | Undo clarity | Mirror simplicity | OS interop | Why |
|---|---|---|---|---|
| Status quo (no device clipboard) | n/a | + | n/a | Rejected — REQ-12 |
| macOS / Qt pasteboard | 0 | − | + | Rejected — PRD Non-Goal; e-ink has no system paste UI |
| Slot as a hidden document node | − | 0 | − | Rejected — would publish and round-trip a clipboard into the file |
| Paste = N× `append_ink` / `create_*` | − (N undos) | 0 | − | Rejected — REQ-12 “paste is one undoable op” |
| **One slot + `duplicate_subtree` (this ADR)** | + | + | − (accepted) | Winner |

Trade-off point: **file-visible clipboard** vs **session slot**. Session slot wins; paste offset is a constant, not a Designer invention.
