---
from: pm
to: architect
date: 2026-08-27
iter: iter-005
verdict: READY-WITH-CONCERNS
cc: [sm, qa]
---

# Hand-off: Product Manager → Architect — adopt inverse-op undo

[CHL-0026](../challenges/CHL-0026-inverse-op-undo.md) (Inverse-op undo, not whole-tree snapshots) is **adopted**. Product fork “snapshots vs inverse” stays **closed**. Do **not** reopen it.

## Verdict

**READY-WITH-CONCERNS.** Product now requires counterpart inverse per device document-epoch, not whole-tree snapshot undo. Software Requirements Specification logic/data/quality and named Architecture Decision Records still describe the old ring until you amend them. That lag is by lock — not a reason to leave [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) (Inverse-op undo per session) proposed.

`adlc prd-check`: **0 FAIL**, 21 WARN (pre-existing open-question owner and other-module coverage; none introduced as FAIL by this adopt).

## Confirmations (binding)

1. **F21 — yes.** Multi-node gesture; some targets **absent**, none **changed** ⇒ apply live targets, no-op absences. Skip the **whole entry** only when any sibling is **changed** (F20). Partial apply is absence-only, never undo-through.
2. **v1 stack — yes.** One device document-epoch stack. Infini is **not** a second author. “Anyone else” = `lastOpId` mismatch, not Infini undo. [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) (Document ownership inversion) §§1–4 stand. Chip chrome ([ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) tiles) stands.

## Must-do rebind (this next Architect run)

You **must** mark ADR-0032 `status: accepted` when you amend. Product Manager did not.

| Artifact | What to do |
|---|---|
| [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) | **Accept** |
| [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) | Amend **§5 only** (snapshot ring + inverse-algebra rationale). Do **not** mark the whole record superseded |
| [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) | Named rows: undo publishes counterpart / `compound`; `restore_snapshot` is not the undo path |
| [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) | Named ring rows (§§3–4). **Tiles stay** |
| [ADR-0024](../../../.docs/adr/ADR-0024-device-clipboard.md) | Named snapshot / `restore_snapshot` rows after paste |
| [ADR-0025](../../../.docs/adr/ADR-0025-barrel-vs-eraser-nib.md) | “one undo restores pre-erase document” → inverse of `set_ink_samples` / `remove_node` |
| [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) | Inverse path; skip/no-op rows |
| [SRS-EP-09](../../../.docs/modules/epaper/features/device-document/srs-data.md#srs-ep-09-device-data) | Undo entry shape `{ forwardOpId, seq, inverses, targets }` |
| [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md) | Skip/no-op quality rows; replace snapshot-cost |
| [SRS-EP-27](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) | Snapshot-backed erase → `set_ink_samples` + inverse |
| [SRS-EP-28](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-28-selection-erase) | Same |
| [SRS-EP-31](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) | Inverse of remove / `duplicate_subtree` |
| [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md) | Add **`compound`** and **`set_ink_samples`** to the transmit set; undo is not wholesale replace |
| [SRS-IN-06](../../../.docs/modules/infini/features/vector-document/srs-quality.md) | Replay of inverse / `compound` equals device tree |
| [domain/vector-document](../../../.docs/domain/vector-document.md) | Inverse ring, depth 20, `lastOpId`; `restore_snapshot` last-resort non-undo |

Chip stories stay on ADR-0018. [SRS-IN-12](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) stays **deprecated**. Do not revive a desktop stack.

Also in ADR-0032 Consequences (do not drop): [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) ops carried; [epaper architecture.md](../../../.docs/modules/epaper/architecture.md) strategy paragraph; glossary **node `lastOpId`** and **session undo stack**.

## Product already updated (do not rewrite)

- [REQ-04](../../../.docs/modules/epaper/prd.md#device-document) (On-device working document) — counterpart + skip/no-op acceptance
- [BR-D05](../../../.docs/modules/epaper/features/device-document/srs-product.md) — no Infini SRS-IN-12 inherit

## Review (review-prd)

### Strengths

- REQ-04 states an **outcome** (counterpart undo, one device stack, skip/no-op) with measurable Given/When/Then — not a field list and not a disguised snapshot.
- F21 and F20 are product-testable without inventing a second author.
- Ownership inversion and ToolChip chrome are explicitly out of this change.

### Concerns (accepted)

- Logic/data/quality and domain still say snapshot until this Architect amend. **By lock.** Implement stories must not AC the old ring.
- `set_ink_samples` is a new wire op; Path A erase cannot ship inverse undo without it. Do **not** fall back to `restore_snapshot`.
- [REQ-08](../../../.docs/modules/epaper/prd.md#node-manipulation) / [REQ-15](../../../.docs/modules/epaper/prd.md#tables) still use “restores” wording — **out of this campaign lock**; do not thicken them here.

### Gaps

None that block accept. Unmeasurable snapshot exactness is gone from REQ-04 / BR-D05.

## Constraints

Vertical, stop `verified`, bounded. **Forbidden:** REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023; TRACK-006 reopen; DeviceMap invert UI; Mouse DragHandler; undo/TRACK-005 feature **code** until inverse-undo ADR accepted (you are accepting it in this next run). Do not start W3. Do not continue TRACK-006.

## Next

Accept ADR-0032 and amend the must-do table. Then `/sm` slices. Do **not** change `epaper/` or `infini/` application code in the amend run.
