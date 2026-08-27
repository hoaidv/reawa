---
from: architect
to: sm
date: 2026-08-27
iter: iter-005
verdict: READY-WITH-CONCERNS
cc: [pm, qa]
---

# Hand-off: Architect → Scrum Master — inverse-op undo accepted

[ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) (Inverse-op undo per session) is **accepted**. Product fork “snapshots vs inverse” stays **closed**. Do **not** reopen it. Do **not** revive Infini [SRS-IN-12](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) (Undo history — deprecated). Chip chrome stays [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) (ToolChip history actions: Undo and Redo). Architect did **not** slice stories, MASTER, the execution board, or tracks. **No** `epaper/` or `infini/` application code.

## Verdict

**READY-WITH-CONCERNS.** Named Software Requirements Specification logic/data/quality, domain, glossary, and Architecture Decision Record sections now describe counterpart inverse per device document-epoch. Implement stories may AC the inverse ring. Remaining concerns are leftover snapshot wording **outside** the must-do table (deprecated Infini authoring rows + BDD) and the Infini applier that must ship in the same slice as device emit.

Binding product confirmations (do not reopen): **F21** absences no-op; any **changed** sibling ⇒ skip whole entry (F20). **v1** one device document-epoch stack; Infini is not a second author; “anyone else” = `lastOpId` mismatch.

## ADR-0032

| Field | Value |
|---|---|
| Status | **accepted** 2026-08-27 |
| Amends | ADR-0014 **§5 only** (ownership §§1–4 stand); ADR-0015 §2; ADR-0018 §§3–4 (tiles stay); ADR-0024 / ADR-0025 named rows |
| Not superseded | ADR-0014 as a whole; ADR-0023 (last-writer) stays superseded and unreopened |

## Files amended (must-do)

| Artifact | Change |
|---|---|
| [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) | `status: accepted`. Proposed banner dropped. Risks row no longer “CHL-0026 open” |
| [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) | §5 inverse-op; amendments table ADR-0013 §5 row **superseded as a mechanism**. Whole record still accepted |
| [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) | Undo publishes counterpart / `compound`; `restore_snapshot` last-resort non-undo; op list adds `compound`, `set_ink_samples` |
| [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) | §§3–4 counterpart redo + publish. Tiles / gap / action-vs-tool **unchanged** |
| [ADR-0024](../../../.docs/adr/ADR-0024-in-document-clipboard.md) | Cut/paste inverses; no snapshot / `restore_snapshot` after paste |
| [ADR-0025](../../../.docs/adr/ADR-0025-barrel-vs-eraser-nib.md) | Pre-erase restore still true; mechanism is inverse of `set_ink_samples` / `remove_node` |
| [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) | Inverse path; skip/no-op; exactness = `lastOpId` match → pre-op fields. Same-file [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-08-one-way-sync) publish row rebound (one line; was live `restore_snapshot`) |
| [SRS-EP-09](../../../.docs/modules/epaper/features/device-document/srs-data.md#srs-ep-09-device-data) | Entry `{ forwardOpId, seq, inverses, targets }` |
| [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md) | Skip/no-op quality rows; entry-size bound; ADR-0032 quality scenarios placed |
| [SRS-EP-27](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md#srs-ep-27-eraser-nib) | `set_ink_samples` + inverse |
| [SRS-EP-28](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-28-selection-erase) | Inverse of `remove_node`; Path A via `set_ink_samples` |
| [SRS-EP-31](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) | Inverse of remove / `duplicate_subtree`; copy = 0 entries |
| [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md) | Transmit set: `compound`, `set_ink_samples`; undo is not wholesale replace |
| [SRS-IN-06](../../../.docs/modules/infini/features/vector-document/srs-quality.md) | Replay of inverse / `compound` equals device tree |
| [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) | Ops carried = same set |
| [domain/vector-document](../../../.docs/domain/vector-document.md) | Inverse ring, depth 20, `lastOpId`; `restore_snapshot` last-resort non-undo |
| [epaper architecture.md](../../../.docs/modules/epaper/architecture.md) | Strategy paragraph; risks row no longer “20 whole-tree snapshots” |
| [glossary.md](../../../.docs/glossary.md) | **node lastOpId**, **session undo stack** |

PRD [REQ-04](../../../.docs/modules/epaper/prd.md#device-document) and [BR-D05](../../../.docs/modules/epaper/features/device-document/srs-product.md) were already product-updated — not rewritten.

## Leftover snapshot wording (do not implement)

These still mention snapshot undo. They are **not** the device ring. Do not AC them as the product.

| Where | What | What to do |
|---|---|---|
| [SRS-IN-12](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) | Deprecated snapshot ring | **Keep deprecated.** Do not revive a desktop stack |
| Infini [srs-logic.md](../../../.docs/modules/infini/features/vector-document/srs-logic.md) `cta.undo` + IN-10/11 Undo rows | “Restore previous snapshot ([SRS-IN-12])” | Point at deprecated IN-12; do **not** slice Infini undo. Optional later retire-in-place |
| [undo-ring.feature](../../../.docs/modules/epaper/features/device-document/bdd/undo-ring.feature) (device) and Infini `bdd/undo-ring.feature` | Pre-op snapshot exactness | **QA retag** after slice: `lastOpId` match → identical fields; skip/no-op consume the entry; 0 `restore_snapshot` on undo |
| Ink-box BDD `enclose-recognition` / `selection-create-surround` | “pre-op snapshot” Then steps | Retag with inverse exactness when those features touch undo |
| [srs-product.md](../../../.docs/modules/epaper/features/device-document/srs-product.md) trace note | Still says Architect will accept ADR-0032 | Product Manager leftover; outcome is already inverse. Do not thicken this run |
| [ADR-0013](../../../.docs/adr/ADR-0013-ink-box-tool-modes.md) §5 | Historical Infini snapshot undo | Body kept (append-only ADR). Mechanism superseded via ADR-0014 amendments table + ADR-0032 |
| [SRS-EP-30](../../../.docs/modules/epaper/features/local-pen-ink/srs-quality.md) | “Undo after Path A → pre-erase document” | Outcome still true; mechanism is inverse (SRS-EP-27). No snapshot word |

No named must-do section was left with live snapshot-ring wording. ADR-0032 Context table still describes the **pre-accept** defect — that is history, not the current spec.

## What Scrum Master should slice next

Do **not** start W3. Do **not** continue TRACK-006. Chip stories stay on ADR-0018.

Replace shipped [STORY-EP-015](../../iter-003/stories/STORY-EP-015.md) (Device undo ring) snapshot behaviour — do not extend it:

1. **Device inverse ring** on `DeviceDocument` — `UndoEntry { forwardOpId, seq, inverses, targets }`; depth 20; one entry per completed gesture.
2. **`lastOpId`** on nodes (counts-as-change list in domain); do not bump derived warp / last-live-pose.
3. **Fail-safe / skip fixtures** — F21 (absences no-op, apply live); F20 (any changed sibling ⇒ skip whole); empty ring no-op; consume on skip/no-op; 0 error UI; 0 redo push for skip/pure no-op.
4. **Infini applier** for `compound` and `set_ink_samples` in the **same** slice as device emit. Unknown op → suspect mirror (existing). Do **not** fall back to `restore_snapshot`.
5. **Publish path** — counterpart / `compound`; **0** `restore_snapshot` on undo/redo. Path A erase = `set_ink_samples` (+ `remove_node` if emptied).
6. **QA** retag `undo-ring.feature`: exactness when rev matches; skip/no-op; 0 wholesale undo.

Honor the counterpart table: ungroup ≠ delete children; connector warp is derived; copy = 0 entries.

## Review (review-design)

### Strengths

- Human lock is the accepted decision: counterpart inverse, one device stack, skip/no-op. Status-quo snapshots remain a **rejected** candidate in ADR-0032 Alternatives.
- Ownership inversion (ADR-0014 §§1–4) and ToolChip tiles (ADR-0018 §§1–2) are explicitly out of the write set.
- Exactness is measurable: `lastOpId` match → pre-op fields (0 divergent; ±1 world unit). Mismatch → skip, not an inexact restore.
- F21 / F20 are testable without a second author. Quality scenarios from ADR-0032 Consequences are in SRS-EP-13.
- `restore_snapshot` kept as last-resort **non-undo** so the type is not silently deleted; undo must not emit it.
- Node `lastOpId` and session undo stack have one domain/glossary home. Infini has no stack.

### Concerns (accepted)

- Infini vector-document logic still has snapshot Undo rows that **point at deprecated SRS-IN-12**. Not a second stack; implementers must not AC them. Slice the device ring + Infini **applier**, not Infini history.
- BDD `undo-ring.feature` still asserts snapshot exactness. QA retag is the next persona, not a spec hole.
- `set_ink_samples` / `compound` are new wire ops. Path A erase cannot ship inverse undo without the Infini applier. Do **not** fall back to `restore_snapshot`.
- [ADR-0024](../../../.docs/adr/ADR-0024-in-document-clipboard.md) / [ADR-0025](../../../.docs/adr/ADR-0025-barrel-vs-eraser-nib.md) remain `status: proposed` as records; only their named undo-mechanism rows were amended. Accept of those ADRs is a separate campaign.

### Risks

| Risk | Quality goal | Mitigation |
|---|---|---|
| Derived connector warp / last-live-pose bumps `lastOpId` → false skips | Correctness | Closed “does not count” list; fixture: drag bound box, undo box, connector still derived |
| Naive `remove_node` of a SmartGroup deletes content inks | Correctness | Ungroup inverse stores child parents; shared fixtures |
| Old Infini build receives `compound` / `set_ink_samples` | Compatibility | Unknown op → suspect mirror (ADR-0015); ship applier in the same slice as device emit |

Sensitivity: **`lastOpId` match** determines apply vs skip. Trade-off: **exact whole-tree restore** vs **non-clobber** — non-clobber wins when they conflict.

## Constraints (lock)

Vertical, stop `verified`, bounded, wip 2, modules epaper + infini. **Forbidden:** REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023; TRACK-006 reopen; DeviceMap invert UI; Mouse DragHandler. Do not start W3. Do not continue TRACK-006.

Architect did not change MASTER, the execution board, or tracks.
