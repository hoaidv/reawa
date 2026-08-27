---
id: CHL-0026
author: sm
target: [SRS-EP-07, ADR-0014, ADR-0015, ADR-0018]
severity: high
status: resolved
resolution: adopted
opened: 2026-08-21
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0026 — Inverse-op undo, not whole-tree snapshots

## Context

Human 2026-08-21, during [TRACK-005](../../tracks/TRACK-005-hand-on-paper.md) (Hand-on-paper). Quoted shipped rationale (“undo is whole-tree snapshots (boring, correct)”) is **wrong**. Change **in this track**. **No application code** until the Architecture Decision Record is accepted and stories are sliced.

Shipped today: [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) §5 (snapshot ring on device), [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) (Device document, ingestion, op set, and undo ring), [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) (`restore_snapshot` publish), [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) (ToolChip history actions: Undo and Redo), [STORY-EP-015](../../iter-003/stories/STORY-EP-015.md) (Device undo ring).

## Human lock (do not reopen the product fork)

| Topic | Decision |
|---|---|
| Inverse | Each forward operation has a counterpart undo (create ink → remove that ink, fail-safe; move A from local (x old, y old) to (x new1, y new1) → move A back to (x old, y old)). |
| Stack | Each **session** has its own undo stack. |
| Fail-safe | If the node was removed by anyone else and cannot move back → **no-op**. If the parent was removed by anyone else and our added node is gone too → **no-op**. Architect investigates fail-safe deeper. |
| No undo-through | If A then moved from (x new1, y new1) to (x new2, y new2) by someone else, skip undo of the earlier move. Safer not to undo-through someone else’s operation. Trick: store the node’s version on the undo entry; if the node has changed since then → skip. Architect investigates undo rules deeper. |

## Proposal

1. Solution Architect drafts a **proposed** Architecture Decision Record (next free id; expected [ADR-0032](../../../.docs/adr/)) that **supersedes** ADR-0014 §5 snapshot mechanism (ownership inversion in ADR-0014 §§1–4 is **not** this challenge).
2. Investigate fail-safe no-op catalogue and no-undo-through / version-skip rules (including tension with ADR-0014 single writer vs “anyone else” / per-session stacks).
3. Do **not** rewrite [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md) or change `epaper/` / `infini/` until Product Manager adopts and Scrum Master slices stories.
4. Chip chrome ([ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md)) stays unless the investigation says otherwise — this challenge is the **ring**, not the tiles.

## Resolution

**Adopted** 2026-08-27 (pm). Human 2026-08-21 closed the product fork (snapshots vs inverse); human 2026-08-27 said **go**. Whole-tree snapshot undo is not the product. Inverse-op per session is.

### Confirmations (binding)

1. **F21 — confirmed.** Multi-node gesture; some targets **absent**, none **changed** ⇒ apply live targets, no-op absences. Skip the **whole entry** only when any sibling is **changed** (F20). Partial apply is absence-only, never undo-through.
2. **v1 stack — confirmed.** One device document-epoch stack. Infini is **not** a second author. “Anyone else” = `lastOpId` mismatch, not Infini undo. [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) §§1–4 stand. Chip chrome ([ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) tiles) stands.

Product Manager does **not** mark [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) `status: accepted`. Architect must accept it when amending the named sections.

## Product doc updates

- `.docs/modules/epaper/prd.md` — [REQ-04](../../../.docs/modules/epaper/prd.md#device-document) undo outcome + acceptance; related snapshot-restore ACs in [REQ-05](../../../.docs/modules/epaper/prd.md#device-ink-box) / [REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation) / [REQ-09](../../../.docs/modules/epaper/prd.md#device-connectors) / [REQ-11](../../../.docs/modules/epaper/prd.md#erase) / [REQ-12](../../../.docs/modules/epaper/prd.md#clipboard) pointed at counterpart + skip/no-op (not field lists). Version `0.13.0-draft`.
- `.docs/modules/epaper/features/device-document/srs-product.md` — [BR-D05](../../../.docs/modules/epaper/features/device-document/srs-product.md) no longer inherits Infini [SRS-IN-12](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) snapshot-ring. Version `0.1.2`.
- Infini PRD — **not edited** (no live REQ claims Infini undo; Infini [SRS-IN-12](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) stays deprecated).

Architect amends SRS logic/data/quality, domain, and named ADRs **after** accepting ADR-0032. Scrum Master slices after that amend — not this run.
