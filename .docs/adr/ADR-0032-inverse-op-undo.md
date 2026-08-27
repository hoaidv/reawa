---
id: ADR-0032
title: Inverse-op undo per session
status: accepted
date: 2026-08-21
accepted: 2026-08-27
deciders: [architect, pm]
supersedes: null
amends: [ADR-0014, ADR-0015, ADR-0018, ADR-0024, ADR-0025]
source: CHL-0026
---

# ADR-0032 — Inverse-op undo per session

**Status: accepted** 2026-08-27. Product Manager adopted [CHL-0026](../../.plan/iter-005/challenges/CHL-0026-inverse-op-undo.md) (Inverse-op undo, not whole-tree snapshots). Named sections in Consequences are amended to this record. Ownership inversion in [ADR-0014](./ADR-0014-document-ownership-inversion.md) §§1–4 and ToolChip tiles in [ADR-0018](./ADR-0018-undo-redo-chip-actions.md) stand.

## Context

Human 2026-08-21: whole-tree snapshot undo is **wrong**. The product fork “snapshots vs inverse” is **closed**. Do not reopen it.

Shipped today, and **wrong as a mechanism** (ownership inversion is not this decision):

| Artifact | What it currently says |
|---|---|
| [ADR-0014](./ADR-0014-document-ownership-inversion.md) §5 | Snapshot ring on the device; inverse-op algebra “gets reparent and coordinate-space changes wrong” |
| [SRS-EP-07](../modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) | Gesture-commit pushes a whole-tree snapshot; undo/redo `restore_snapshot` |
| [ADR-0015](./ADR-0015-one-way-sync-contract.md) §2 | `restore_snapshot` is how undo publishes |
| [ADR-0018](./ADR-0018-undo-redo-chip-actions.md) §§3–4 | Snapshot redo stack; publish undo/redo as `restore_snapshot` |
| [ADR-0025](./ADR-0025-barrel-vs-eraser-nib.md) | “One undo restores pre-erase document” via snapshot-backed mutation |
| [domain/vector-document](../domain/vector-document.md) | `restore_snapshot` = wholesale replace; device undo = snapshot ring |
| [STORY-EP-015](../../.plan/iter-003/stories/STORY-EP-015.md) | Shipped snapshot ring on `DeviceDocument` |

[ADR-0014](./ADR-0014-document-ownership-inversion.md) §§1–4 **stay**: the device is the sole writer of the working document during a live session; Infini writes nothing into that document; the panel paints the device tree; the desktop holds a mirror plus the file. Chip chrome in ADR-0018 (Undo / Redo as actions, not tools; 32 du gap; tiles 64×64) **stays**. This record replaces the **ring**, not the tiles.

Human lock (do not weaken):

1. Each forward operation has a counterpart undo (create ink → remove that ink, fail-safe; move A from local `(x old, y old)` to `(x new1, y new1)` → move A back to `(x old, y old)`).
2. Each **session** has its own undo stack.
3. Fail-safe **no-op** if the node or parent was removed by anyone else and the inverse cannot apply.
4. **No undo-through** someone else’s operation. Store the node’s version on the undo entry; if the node has changed since then → skip.

Quality goals at stake ([epaper REQ-04](../modules/epaper/prd.md#device-document), [REQ-07](../modules/epaper/prd.md#one-way-sync); [SRS-EP-13](../modules/epaper/features/device-document/srs-quality.md)):

| Priority | Attribute | Bar this ADR must not regress |
|---|---|---|
| 1 | Correctness (tree + concurrency) | 0 invariant violations; skip is not an error; 0 undo-through |
| 2 | Recoverability | 1 undo entry per completed gesture; depth ≥20; empty is a no-op |
| 3 | Performance / payload | Ink p95 ≤30 ms; undo publish p95 ≤300 ms mirror; inverse payload ≪ whole tree |
| 4 | Implementability | Device can compute inverses locally with the link down ([BR-D04](../modules/epaper/features/device-document/srs-product.md)) |

ADR-0014 §5’s rationale (inverse algebra “gets reparent wrong”) is **rejected**. The mitigation is not snapshots; it is storing **absolute pre-op parent, index, and field values** on the entry, then applying them only when the node revision still matches.

### Quality utility tree (this decision)

| Attribute | Sub-concern | Scenario leaf | Biz × Arch |
|---|---|---|---|
| Correctness | Fail-safe | Absent node/parent → no-op; tree still valid | H × H |
| Correctness | No undo-through | Later other-op on the same node → skip; that later op remains | H × H |
| Recoverability | Gesture unit | One tap undoes one completed gesture, not a sample | H × M |
| Performance | Wire size | Undo `doc_change` comparable to the forward op, not the whole document | M × H |
| Reliability | Skip ≠ error | 0 error UI; 0 published ghost change | H × L |
| Maintainability | One history model | Redo is the counterpart of inverse, not a third log | M × M |

High×high leaves are the ones this ADR must earn. Measurable bars live in [SRS-EP-13](../modules/epaper/features/device-document/srs-quality.md) (amended on accept).

## Decision

**Undo is the counterpart inverse of each committed gesture, stacked per session, published as ordinary tree ops. Whole-tree snapshots are not the undo mechanism.**

`restore_snapshot` remains in the transmit grammar as a **last-resort, non-undo** wholesale replace (emergency / tests). Undo and redo **must not** emit it.

### 1. Session (who has a stack)

| Term | Meaning in v1 |
|---|---|
| **Document epoch** | From an accepted `doc_load` (or boot with an empty tree) until the next accepted `doc_load` or process death. `seq` resets on load ([ADR-0015](./ADR-0015-one-way-sync-contract.md) §4). |
| **Undo session** | One document epoch on **one writer**. v1 has exactly one writer: the device ([ADR-0014](./ADR-0014-document-ownership-inversion.md) §1). |
| **Session stack** | That writer’s undo ring + redo stack for this epoch. Depth **20** each. Accepted `doc_load` **empties both**. App restart discards both (already accepted: in-memory document). |

Infini keeps **no** undo stack while it is a follower ([SRS-IN-12](../modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) stays deprecated). Giving the desktop a stack would be a second writer and would contradict ADR-0014 §§1–4.

**“Anyone else” is not a second live author today.** It is a **predicate on node revision**, not a peer identity and not a last-writer token ([ADR-0023](./ADR-0023-viewport-last-writer.md) stays superseded and is out of this decision). A mutation is “someone else’s” relative to an undo entry when that node’s last mutating `opId` is **not** the forward `opId` stored on the entry.

This is latent in v1 (single LIFO stack, single writer): sequencing is LIFO, so a later *own* op is undone first. The revision check is the safety net that (a) refuses to clobber a node that changed by a path the stack did not expect, (b) survives id-reuse bugs, and (c) is the rule a future second writer-session can use **without** inventing CRDT/OT or reopening document last-writer. Two-way document sync remains a Non-Goal.

### 2. Inverse algebra (not symbolic)

An undo entry is **not** a whole-tree snapshot. It is the committed gesture’s inverse, stored as concrete values:

```text
UndoEntry {
  forwardOpId,                 # opId of the committed gesture
  seq,                         # device seq of that commit
  inverses: [ Inverse ],       # 1..N counterpart edits, apply order
  targets:  [ { nodeId, prevLastOpId } ]  # lastOpId on each node *before* this forward edit
}
```

`Inverse` is an ordinary tree op (same names as [SRS-IN-09](../modules/infini/features/vector-document/srs-data.md) transmit ops) whose payload holds **absolute** pre-op values: parent id, sibling index, transform, samples, style, etc. Do not invert a matrix chain or “guess” the previous parent.

One completed **gesture** remains one undo entry ([REQ-04](../modules/epaper/prd.md#device-document), [BR-D05](../modules/epaper/features/device-document/srs-product.md)). Intermediate manipulation frames still do not push. Viewport, tool, selection, and clipboard-slot contents still do not push (copy remains 0 entries).

#### Counterpart table

| Forward (gesture) | Inverse | Notes |
|---|---|---|
| `append_ink` / `create_primitive` / `create_frame` / `create_connector` | `remove_node` of that id | Fail-safe if absent |
| `create_smart_group` | **Ungroup**: reparent pre-existing children to stored `{parentId, index}`; `remove_node` the group (boundary ink was created → it goes with the group) | Not “delete group and children”. Store each child’s pre-enclose parent/index |
| `set_smart_transform` | `set_smart_transform` with **stored old** transform/bounds | Absolute old pose, not a delta |
| `set_ink_scale_mode` | restore stored mode | |
| `reparent` | `reparent` to stored `{oldParentId, oldIndex}` | |
| `remove_node` (incl. cut, selection-erase) | restore stored node body at stored `{parentId, index}` | Body travels on the undo entry, not on the wire until undo applies |
| Empty-group cleanup (same gesture as last-child remove) | restore group body then child body | One entry; stored as two inverses |
| `duplicate_subtree` (paste) | `remove_node` each pasted id | Slot **unchanged** (clipboard is session state, not document) |
| Copy | — | 0 entries |
| `set_connector_end_style` | restore stored style on that end | |
| `bind_endpoint_ink` | restore previous `terminal[end].ink` (possibly empty) | |
| `bind_attachment` | unbind that `{nodeId, t}` | |
| Stroke-erase / sample edit | `set_ink_samples` with stored previous samples; if the forward also `remove_node`’d an emptied ink, restore that body | **Requires** `set_ink_samples` on the wire (see §4) |
| Bound-node drag (connector warp) | Inverse of `set_smart_transform` only | Warp is **derived** ([ADR-0020](./ADR-0020-connector-ink-geometry.md) I1). Live re-warp frames are not undo units. Rest spine is not rewritten, so restoring the box pose re-derives `V` |

Multi-node gestures (selection move of N boxes, selection-erase of N nodes, multi-node paste): **one entry**, N inverses, applied as one commit.

### 3. Node version (what “changed” means)

Each node carries `lastOpId` — the `opId` of the last **forward** document-semantic mutation still in effect on that node. New nodes start at the creating op’s `opId`. Undo restores `lastOpId` to `prevLastOpId` captured at commit (the previous forward). Redo restamps the original `forwardOpId`. Wire envelopes for history may use `undo:N` / `redo:N` as publish `opId` only — those ids are **not** written onto nodes.

**Counts as a change** (must update `lastOpId`):

- Parent id or sibling index
- Geometry: transform, bounds, ink samples (including erase), `inkScaleMode`, `layoutOffset`
- Connector stored fields: `from` / `to` anchors, rest shape `S`, `warpStyle`, `terminal` style/ink, attachments list
- Text runs, primitive geometry, kind-specific author fields

**Does not count** (must **not** update `lastOpId`):

- Derived paint, hit caches, warped polyline `V`, attachment world pose
- Connector **last-live-pose** cache when an endpoint is missing ([SRS-EP-18](../modules/epaper/features/connector-ink/srs-logic.md#srs-ep-18-connector-warp))
- Selection, tool mode, viewport, follow, publish-queue metadata
- Clipboard slot
- `seq` on the envelope (the node stores `lastOpId`, not `seq`)

**Id reuse:** do **not** reuse a node id inside an epoch. Across epochs the stacks are empty, so a file-level id that reappears after `doc_load` cannot meet an old entry. If a restore inverse finds its `nodeId` **already present**, that is a skip (do not clobber), even if `lastOpId` were somehow equal.

Undo-time check, per target:

| Observed | Class | Action |
|---|---|---|
| Node absent (and inverse needed it present) | **Fail-safe** | That inverse is a **no-op** |
| Required parent absent and the node is absent too | **Fail-safe** | **No-op** |
| Node present and `lastOpId` ≠ entry’s forward `opId` | **Undo-through** | **Skip** |
| Node present and `lastOpId` == entry’s forward `opId` | Apply | Run the inverse; then restore `lastOpId` to that target’s `prevLastOpId` |

#### Atomic gesture rule

1. Classify every target: apply / no-op / skip.
2. **If any target is skip → apply nothing.** Consume the entry. Do not half-undo a multi-node gesture (that would yank sibling B while leaving someone else’s later edit of A).
3. Else apply the apply-set; absences stay no-ops. If the apply-set is empty, the whole tap is a no-op.
4. After the batch, domain invariants must hold. An inverse that would orphan a child, insert under a missing parent, or revive a SmartGroup with zero children **does not apply** (treat as no-op), and **must not** “repair” by deleting unrelated nodes.

Skip and no-op are **success paths**: 0 error UI, 0 tree corruption, 0 `doc_change`. Consume the undo entry so the creator is not stuck tapping a dead top-of-stack. **Do not** push redo for a skip or a pure no-op.

LIFO inside one session remains the sequencer. Version skip is not an alternative history model.

### 4. How undo publishes

Local apply first (link down is normal). If at least one inverse **applied**, enqueue **one** `doc_change` for that undo gesture ([ADR-0015](./ADR-0015-one-way-sync-contract.md) one-change-per-gesture). The `op` is the counterpart, not a tree dump.

| Inverse shape | Wire `op` |
|---|---|
| Single counterpart (`remove_node`, `set_smart_transform`, …) | That op type |
| Several counterparts in one gesture | `compound { ops: [ … ] }` — atomic on the mirror; unknown type still marks the mirror **suspect** (existing rule) |

Do **not** emit N `doc_change` messages for one undo tap.

**`set_ink_samples { id, samples }`** is on the transmit set so Path A erase has a real inverse. Stroke-erase must not cheat via `restore_snapshot`.

`restore_snapshot` after accept:

- **Not** used for undo or redo
- May remain in the grammar for wholesale replace outside history (tests, future emergency). Product Manager may retire it later; this ADR does not delete the type

Mirror apply of an inverse is the same idempotent-by-`opId` path as any other op. A skipped/no-op undo produces **0** messages, so the mirror does not need a “skip” op.

### 5. Redo (counterpart, not a third model)

Keep linear redo ([ADR-0018](./ADR-0018-undo-redo-chip-actions.md) chrome and the product “Redo exists; branching history does not” rule).

| Rule | Value |
|---|---|
| What redo stores | The **forward** ops just inverted (inverse-of-the-inverse), plus the same `targets` / original `forwardOpId`. Skip redo when a live node’s `lastOpId` ≠ that target’s `prevLastOpId` (the lastOpId undo restored). After a successful redo, stamp `lastOpId` = `forwardOpId` again. Do **not** replace `forwardOpId` with `redo:N`. |
| Depth | 20 |
| Successful structural commit | **Clears** redo (unchanged) |
| Successful undo that applied ≥1 inverse | Pushes one redo entry |
| Skip / pure no-op undo | No redo push |
| Redo apply | Same fail-safe / skip / atomic-gesture rules |
| Empty redo | No-op |
| Mid-gesture | Latch; last history request wins; run after commit (unchanged) |
| Publish | Counterpart ops / `compound`, never `restore_snapshot` |

There is one history model: a pair of LIFO stacks of concrete inverses. Redo is not snapshots, not a branch, and not a CRDT tombstone log.

### 6. Fail-safe catalogue (required investigation)

| # | Case | Class | Tree | Publish |
|---|---|---|---|---|
| F1 | Empty undo or redo | No-op | Unchanged | 0 |
| F2 | Target node removed by a later op / anyone else; inverse needed it | No-op | Unchanged | 0 |
| F3 | Parent of an add is gone **and** our node is gone | No-op | Unchanged | 0 |
| F4 | Parent of a restore is gone; node body would be orphaned | No-op (do not insert at root as a “fix”) | Unchanged | 0 |
| F5 | Reparent inverse; old parent missing | No-op | Unchanged | 0 |
| F6 | Reparent / transform / style; node present but `lastOpId` ≠ forward | Skip | Unchanged | 0 |
| F7 | Create inverse (remove); node present but later mutated | Skip (do not delete through their edit) | Unchanged | 0 |
| F8 | Resize then someone else resized again | Skip | Unchanged | 0 |
| F9 | Sample erase; node gone | No-op | Unchanged | 0 |
| F10 | Sample erase; later sample edit on same ink | Skip | Unchanged | 0 |
| F11 | Sample erase emptied the ink and forwarded as `remove_node`; id later reused | Skip (id occupied) | Unchanged | 0 |
| F12 | Cut undo (restore originals); originals’ ids occupied | Skip | Unchanged | 0 |
| F13 | Paste undo; copies already removed | No-op those ids | Slot unchanged | 0 if nothing applied |
| F14 | Clipboard slot is not a node | Never on the stack | Slot not versioned | — |
| F15 | Connector warp during bound-node drag | Not an entry | Derived `V` only | — |
| F16 | Undo bound-node move; connector was deleted | Apply box inverse; connector absence is connector fail-safe, not a skip of the box | Box restored; missing connector stays missing | Box op only |
| F17 | Undo `create_connector`; connector restyled or rebound | Skip | Connector stays | 0 |
| F18 | Undo delete of a bound endpoint; connector used last-live-pose | Restore node; live-resolve again ([SRS-EP-18](../modules/epaper/features/connector-ink/srs-logic.md#srs-ep-18-connector-warp)); pose cache must not have bumped `lastOpId` | Node back; connector live | restore op |
| F19 | Empty-group cascade in one gesture; group body restore + child restore | Apply both or no-op both if parent missing | No empty SmartGroup left behind | compound or 0 |
| F20 | Multi-node gesture; any target is skip | Skip whole entry | Unchanged | 0 |
| F21 | Multi-node gesture; some targets absent, none skip | Apply the live targets; no-op the absences | Valid; partial only for **absence**, never for **changed** | 1 if any applied |
| F22 | Opaque / desktop-only kinds we did not author this epoch | Not on our stack | Cannot undo their creation; we **can** undo our later transform of them if we did one | — |
| F23 | Accepted `doc_load` | Stacks emptied | Cannot undo through the load | — |
| F24 | Unpublished undo, then app restart | Stacks gone | Accepted (in-memory document) | — |
| F25 | Unknown inverse op on the mirror | Existing unknown-op rule | Mirror **suspect**; do not save silently | — |

Fail-safe no-op **must not** corrupt the tree: 0 half-inserted nodes, 0 empty `Group` / `SmartGroup` left invalid, 0 sibling-order salvage that rewrites unrelated nodes.

### 7. Chip chrome

[ADR-0018](./ADR-0018-undo-redo-chip-actions.md) layout and action-vs-tool split **unchanged**. Only the ring behind `cta.undo` / `cta.redo` changes. Empty remains a no-op. Investigation did not produce a reason to move or drop the tiles.

## Consequences

### On accept (done 2026-08-27)

**ADR-0014 §5 superseded by this ADR**; ADR-0014 is **not** `status: superseded` (ownership inversion stands). Named sections amended:

| After accept | Section | Change |
|---|---|---|
| ADR-0014 | §5 Undo lives where editing lives | Inverse-op per session; drop snapshot-ring and the “inverse gets reparent wrong” rationale |
| ADR-0014 | Amendments table, ADR-0013 §5 row | No longer “kept, re-homed” snapshots |
| ADR-0015 | §2 `restore_snapshot` paragraph; Risks row on large restore ops | Undo publishes counterpart / `compound`; `restore_snapshot` not the undo path |
| ADR-0015 | §2 op list | Add `compound`, `set_ink_samples`; keep `restore_snapshot` as last-resort non-undo or retire by a later PM edit |
| ADR-0018 | §§3–4 | Redo stores forward counterparts; publish like §4 of this ADR; chrome unchanged |
| ADR-0024 | Undo / wire rows that say snapshot or `restore_snapshot` after paste | Cut/paste inverses as in the counterpart table |
| ADR-0025 | “one undo restores pre-erase document” | Still true; mechanism is inverse of `set_ink_samples` / `remove_node`, not a snapshot |
| [SRS-EP-07](../modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) | Gesture-commit, Undo, Redo, op-set `restore_snapshot` row | Inverse path; exactness = “rev matches → pre-op fields restored”; skip/no-op rows |
| [SRS-EP-09](../modules/epaper/features/device-document/srs-data.md#srs-ep-09-device-data) | Undo ring entry shape | `{ forwardOpId, seq, inverses, targets }` not `{ snapshot, opId, kind }` |
| [SRS-EP-13](../modules/epaper/features/device-document/srs-quality.md) Undo table | Exactness + snapshot-cost rows | Add skip/no-op measures; replace snapshot-cost with entry-size bound |
| [SRS-EP-28](../modules/epaper/features/device-document/srs-logic.md#srs-ep-28-selection-erase) / [SRS-EP-27](../modules/epaper/features/local-pen-ink/srs-logic.md) | Snapshot-backed erase | `set_ink_samples` + inverse |
| [SRS-EP-31](../modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) | Snapshot wording | Inverse of remove / `duplicate_subtree` |
| [SRS-IN-09](../modules/infini/features/vector-document/srs-data.md) | Transmit ops + `restore_snapshot` note | `compound`, `set_ink_samples`; undo is not wholesale replace |
| [SRS-IN-06](../modules/infini/features/vector-document/srs-quality.md) | Restore-snapshot apply row | Replay of inverse / `compound` equals device tree |
| [SRS-IN-07](../modules/infini/features/tablet-sync/srs-logic.md) | Ops carried | Same set |
| [domain/vector-document](../domain/vector-document.md) | Operations `restore_snapshot`; Two implementations Undo row | Inverse ring, depth 20, `lastOpId` |
| [epaper architecture.md](../modules/epaper/architecture.md) | Strategy paragraph (“undo ring is whole-document snapshots”) | Point at this ADR |
| Glossary | — | Add **node `lastOpId`**, **session undo stack** after accept |

[SRS-IN-12](../modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) stays **deprecated**. Do not revive a desktop stack.

### Scrum Master (slice after this accept — Architect does not slice)

Replace snapshot-ring stories (starting from shipped [STORY-EP-015](../../.plan/iter-003/stories/STORY-EP-015.md) behaviour) with implement slices for: inverse entries on `DeviceDocument`, `lastOpId`, fail-safe/skip fixtures, `compound` + `set_ink_samples` on the Infini applier, publish path without `restore_snapshot`. Chip stories stay on ADR-0018. QA retags [undo-ring.feature](../modules/epaper/features/device-document/bdd/undo-ring.feature) exactness: “rev matches → identical fields”, plus skip/no-op scenarios.

### Quality scenarios (placed in [SRS-EP-13](../modules/epaper/features/device-document/srs-quality.md) on accept)

| Source | Stimulus | Artifact | Environment | Response | Measure |
|---|---|---|---|---|---|
| Creator | Undo, target absent | Device tree | Live session | No-op, consume entry | 0 divergent extra nodes; 0 error UI |
| Creator | Undo, `lastOpId` mismatch | Device tree | Live or (future) second writer | Skip whole gesture | 0 undo-through; later op’s fields unchanged |
| Creator | Undo, rev matches | Device tree | Normal | Counterpart applied | 0 divergent nodes vs stored pre-op fields; geometry ±1 world unit |
| Creator | Undo mid-gesture | In-flight stroke | Normal | Latch; 0 corrupt gesture | **0** (existing [SRS-EP-13](../modules/epaper/features/device-document/srs-quality.md)) |
| Device | Undo publish | `doc_change` | Link up | Counterpart / `compound`, not wholesale tree | p95 commit → mirror ≤300 ms; payload order-of-forward-op |
| Device | Undo with document resident | Ink path | 500-node fixture | Ring must not steal the ink budget | p95 pen-down → pixel ≤30 ms |

### Trade-off points pinned

1. **Exact whole-tree restore** vs **safe inverse under later edits.** Exactness holds **when `lastOpId` matches**. When it does not, skip wins over clobber. That is the point of the human lock.
2. **Single writer now** vs **per-session stacks later.** v1 does not add a second author. The stack is already per epoch; revision skip is identity-free so a later writer-session can exist without last-writer tokens.
3. **Small ops on the wire** vs **trivial snapshot publish.** Payload and mirror-replay win; implementability pays the catalogue cost.
4. **Redo kept** vs **drop redo.** Chrome already ships Redo; dropping it would be a product change this ADR is not authorised to make. Redo is the counterpart, not a third model.

### Risks

| Risk | Likelihood × impact | Mitigation or accept |
|---|---|---|
| Inverse of `create_smart_group` / empty-group cleanup implemented as naive `remove_node` (deletes children) | M × H | Store pre-op child parents; ungroup ≠ delete children. Shared fixtures |
| `lastOpId` bumped by derived warp / last-live-pose → false skips | M × H | Closed “does not count” list; tests on connector drag + undo box |
| `compound` unknown to an old Infini build | M × M | Unknown op → suspect mirror (existing); ship applier in the same slice as device emit |
| Path A erase blocked on `set_ink_samples` | H × M | Named follow-up; do not fall back to `restore_snapshot` |
| Memory still large if bodies of huge removes sit on the ring | L × M | Depth 20; measure; still far below 20 whole trees |
| Readers treat snapshot-ring SRS as still in force | L × H | Status **accepted** 2026-08-27; CHL-0026 adopted; named sections amended this run |

## Alternatives Considered

Prioritised scores: **Correctness under concurrency** · **Payload size** · **Recoverability** · **Implementability**. `+` better, `0` neutral, `−` worse.

| Approach | Concurrency | Payload | Recoverability | Implementability | Why |
|---|---|---|---|---|---|
| **A. Whole-tree snapshots (status quo)** | − undo-through by construction: restore clobbers later ops on other nodes *and* the same node | − 20× document | + exact always | + already shipped ([STORY-EP-015](../../.plan/iter-003/stories/STORY-EP-015.md)) | **Rejected — human lock.** This is the candidate ADR-0014 §5 chose for “boring, correct.” It is correct only in a single-writer LIFO world with no “anyone else,” and it is the wrong publish unit. |
| **B. Inverse-op per session + `lastOpId` skip (this ADR)** | + skip / fail-safe catalogue | + per-op | + per-gesture when rev matches; skip/no-op when not | 0 new algebra + `compound` / `set_ink_samples` | **Chosen.** Serves correctness-under-later-edits first, then payload. |
| **C. Inverse-op, always apply (no version)** | − teleport / delete-through | + | − | + simpler | Rejected — violates no-undo-through. Reparent “wrong” returns if we apply blind. |
| **D. CRDT / OT undo now** | + | + | 0 different UX | − large; deferred Non-Goal | Rejected for this campaign. Per-session stacks + `lastOpId` are a subset; nothing here has to be undone to get there. |
| **E. Drop undo; snapshots only for crash** | n/a | + | − fails REQ-04 | + | Rejected — recoverability is a Must. |
| **F. Keep snapshots locally, publish inverse** | − local undo still clobbers | + on the wire only | + local exact | − two mechanisms | Rejected — two history models. The device would still undo-through. |
| **G. Drop redo; inverse undo only** | + same as B | + | − vs shipped chrome | + | Rejected — ADR-0018 already required Redo; this ADR is the ring, not the tiles. |

Sensitivity: **whether `lastOpId` matches** fully determines the correctness response (apply vs skip). Trade-off: **exactness** vs **non-clobber**. The lock picks non-clobber when they conflict.

## Amendments (effective on accept)

| Prior clause | After this ADR |
|---|---|
| ADR-0014 §5 snapshot ring + inverse-algebra rationale | **Superseded** by §§1–5 of this ADR. §§1–4 and §6–7 of ADR-0014 **unchanged** |
| ADR-0015 §2 `restore_snapshot` is how undo publishes | **Amended** — undo/redo publish counterpart ops / `compound` |
| ADR-0018 §§3–4 snapshot redo + restore publish | **Amended** — counterpart redo; chrome (§§1–2) **unchanged** |
| ADR-0013 §5 snapshot undo (already re-homed by ADR-0014) | **Superseded** as a mechanism; depth 20 and “not covered” (viewport/tool/selection) **kept** |
| Domain `restore_snapshot` as undo | **Amended** — last-resort wholesale replace, not history |
