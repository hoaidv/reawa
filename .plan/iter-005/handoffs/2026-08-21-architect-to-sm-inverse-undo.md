---
from: architect
to: sm
date: 2026-08-21
iter: iter-005
verdict: READY-WITH-CONCERNS
cc: [pm, qa]
---

# Hand-off: Architect → Scrum Master — inverse-op undo (proposed)

Stories, MASTER, execution board, [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md), [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md), [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) body, and application code were **not** edited. Challenge [CHL-0026](../challenges/CHL-0026-inverse-op-undo.md) stays **open** (Product Manager triage / adopt).

## Verdict

**READY-WITH-CONCERNS.** The product fork is closed in a proposed Architecture Decision Record. Scrum Master must **not** slice implement stories until Product Manager adopts and the named Software Requirements Specification sections are amended. Chip chrome is unchanged.

## ADR

| ID | Title | Status |
|---|---|---|
| [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) | Inverse-op undo per session | **proposed** — do not mark accepted in this run |

**Decision:** each committed gesture stores a concrete counterpart inverse on a **per-session** stack; undo/redo publish ordinary tree ops (`compound` when a gesture has several counterparts). Whole-tree snapshots are **rejected**. `restore_snapshot` is not the undo path (last-resort wholesale replace only).

**Amends (on accept, not now):** ADR-0014 **§5 only** (ownership §§1–4 stand); ADR-0015 §2 publish; ADR-0018 §§3–4 ring (tiles stay); ADR-0024 / ADR-0025 snapshot wording. Domain + [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md) / [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md) sections listed in the Architecture Decision Record Consequences.

## Surprise — single writer vs “anyone else”

[ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) §1 still holds: **the device is the only writer during a live session.** Infini has **no** undo stack ([SRS-IN-12](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) stays deprecated). v1 therefore has **one** live stack: the device’s document epoch (accepted `doc_load` → next load / process death).

“Someone else” is **not** a second Infini author and **not** a last-writer token ([ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) stays superseded). It is a **`lastOpId` mismatch** on the node. Sequencing inside a session is still LIFO; skip is the safety net (and the rule a future writer-session can use without two-way sync, which remains a Non-Goal).

## Next (persona order)

1. **Product Manager** — triage [CHL-0026](../challenges/CHL-0026-inverse-op-undo.md): adopt → accept ADR-0032 and amend the named sections (including `set_ink_samples` + `compound` on the transmit set, skip/no-op quality rows). Confirm F21 (multi-node gesture: absences no-op, **changed** sibling ⇒ skip whole entry).
2. **Scrum Master** — only after accept: slice device inverse ring + Infini applier; **do not** retarget chip stories off [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md). Shipped [STORY-EP-015](../../iter-003/stories/STORY-EP-015.md) snapshot behaviour becomes the thing to replace, not to extend.
3. **QA** — after slice: retag `undo-ring.feature` (exactness when `lastOpId` matches; skip/no-op consume the entry; 0 `restore_snapshot` on undo).

Do **not** rollup. Do **not** change `epaper/` or `infini/`.

## Review (review-design)

### Strengths

- Human lock is recorded as the decision, not as an open fork. Status-quo snapshots are a **rejected** candidate with scores (concurrency −, payload −).
- Ownership inversion ([ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) §§1–4) and ToolChip actions ([ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) §§1–2) are explicitly out of the write set.
- Fail-safe catalogue F1–F25 covers reparent, resize, erase samples, clipboard, connector warp (derived, not an entry), id reuse, empty ring, empty-group cascade, multi-node atomicity.
- Version rule is identity-free (`lastOpId`), so it does not smuggle last-writer back onto the document.
- One history model: redo is inverse-of-the-inverse; skip/no-op do not push redo and do not publish.
- Inverse payloads store **absolute** pre-op parent/index/fields — this is the answer to ADR-0014 §5’s “reparent gets inverse wrong” rationale, which the human rejected.

### Concerns (accepted unless Product Manager objects)

- Software Requirements Specification and domain still describe snapshots until adopt. **By lock.** Implement stories must not AC the old ring while CHL-0026 is open.
- `set_ink_samples` is a **new** wire op; Path A erase cannot ship inverse undo without it. Do not fall back to `restore_snapshot`.
- “Anyone else” is **latent** in v1 (single writer + LIFO). The Architecture Decision Record is honest about that; Product Manager should not rewrite copy as if Infini already undoes.
- Quality scenarios live in the Architecture Decision Record Consequences, not yet in [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md) / [SRS-IN-06](../../../.docs/modules/infini/features/vector-document/srs-quality.md) — Product Manager/Architect after accept.
- F21 (partial apply when some targets are **absent** but none **changed**) is a recoverability choice; skip-whole is reserved for undo-through. Confirm on adopt.

### Risks

| Risk | Quality goal | Mitigation |
|---|---|---|
| Derived connector warp / last-live-pose bumps `lastOpId` → false skips | Correctness | Closed “does not count” list; fixture: drag bound box, undo box, connector still derived |
| Naive `remove_node` of a SmartGroup deletes content inks | Correctness | Ungroup inverse stores child parents; shared fixtures |
| Old Infini build receives `compound` / `set_ink_samples` | Compatibility | Unknown op → suspect mirror (existing ADR-0015); ship applier in the same slice |

Sensitivity: **`lastOpId` match** determines apply vs skip. Trade-off: **exact whole-tree restore** vs **non-clobber** — non-clobber wins when they conflict.

## Constraints (lock)

Vertical, stop `verified`, bounded, wip 2, modules epaper + infini. **Forbidden:** REQ-15, REQ-08, CHL-0011, CHL-0012, EP-032, AI, last-writer ADR-0023, application code. This run: proposed ADR only.
