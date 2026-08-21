---
from: sm
to: architect
date: 2026-08-21
iter: iter-005
cc: [pm]
---

# Hand-off: Scrum Master → Solution Architect — inverse-op undo Architecture Decision Record

Human 2026-08-21: whole-tree snapshot undo is **wrong**. Change **in TRACK-005**. **No application code.** Challenge: [CHL-0026](../challenges/CHL-0026-inverse-op-undo.md).

## Human lock (product fork is closed)

- Each forward operation has a counterpart undo (create ink → remove that ink, fail-safe; move A local (x old, y old) → (x new1, y new1) undoes to (x old, y old)).
- Each session has its own undo stack.
- Fail-safe **no-op** if the node or parent was removed by anyone else.
- **No undo-through** someone else’s operation. Store node version on the undo entry; if the node changed → skip.
- Architect investigates fail-safe and undo-through **deeper** in the Architecture Decision Record.

## Write set

- `python3 .agent/tools/adlc new adr inverse-op-undo --title "Inverse-op undo per session"` (expected ADR-0032).
- Status **proposed**. Do not mark accepted. Do not edit `epaper/`, `infini/`, stories, Master Plan, or execution board.
- Do **not** rewrite [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md) yet (challenge open). You may name which sections the Architecture Decision Record will supersede / amend.
- Ownership inversion [ADR-0014](../../../.docs/adr/ADR-0014-document-ownership-inversion.md) §§1–4 stays. §5 snapshot ring is the wrong mechanism.
- Investigate tension: “anyone else” + per-session stacks vs ADR-0014 **single writer** (device) during a live session.

Handoff back: `.plan/iter-005/handoffs/2026-08-21-architect-to-sm-inverse-undo.md`
