---
id: CHL-0026
author: sm
target: [SRS-EP-07, ADR-0014, ADR-0015, ADR-0018]
severity: high
status: open
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
<!-- PM fills after triage: adopted | deferred | rejected -->

## Product doc updates
<!-- Architect / Product Manager after ADR accept -->
