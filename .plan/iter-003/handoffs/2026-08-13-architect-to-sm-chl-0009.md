---
from: architect
to: sm
date: 2026-08-13
iter: iter-003
source: CHL-0009
verdict: READY-WITH-CONCERNS
---

# Hand-off: Architect → SM — CHL-0009 device-document srs-logic

## Context

[CHL-0009](../challenges/CHL-0009-missing-device-document-srs-logic.md) is **adopted**
(completeness gap, not a direction change). The missing file is on disk with the ids
already assigned in the 2026-08-13 device-document handoff. No new ids. ADRs unchanged.

| Artifact | Path |
|---|---|
| Logic SRS | [`.docs/modules/epaper/features/device-document/srs-logic.md`](../../../.docs/modules/epaper/features/device-document/srs-logic.md) |
| `[SRS-EP-07]` | `{#srs-ep-07-device-document}` — tree, ingestion, op set, undo ring. Parent `[REQ-04]` |
| `[SRS-EP-08]` | `{#srs-ep-08-one-way-sync}` — inbound classification, load handshake, publish queue, preview. Parent `[REQ-07]` |

BDD already tagged is now heading-resolvable: `undo-ring.feature` `@SRS-EP-07` (8 scenarios),
`one-way-sync.feature` `@SRS-EP-08` (11 scenarios). `adlc audit`: both **covered**.

## Review verdict: READY-WITH-CONCERNS

Self-check against `review-design.md` / architecture principles. Applicable areas only.

| Class | Finding | Evidence |
|---|---|---|
| Strength | Every existing BDD scenario has a named algorithm (gesture-commit path, inbound table, handshake, mid-gesture load deferral). QA does not need new journeys | `srs-logic.md` ↔ `bdd/undo-ring.feature` / `bdd/one-way-sync.feature` |
| Strength | Wire grammar is bound, not forked; quality numbers stay in `[SRS-EP-13]` | SRS-IN-09 envelopes · SRS-EP-13 references only |
| Strength | No peer round trip inside a gesture is an explicit EP-07 invariant; load-during-gesture is deferred until commit + publish | ADR-0014 §1 · ADR-0015 §4 |
| Concern (accepted) | Op type aliases already exist across docs (`set_smart_transform` / `set_smart_group_transform`, `remove_node` / `remove`). This file uses the **SRS-IN-09 transmit names**. Do not let an implement story invent a third | SRS-IN-09 vs domain §Operations vs SRS-IN-07 “Ops carried” |
| Risk | None introduced by this file. Device-capacity (ink budget with a resident tree) remains the campaign risk and is owned by **EP-013**, still in flight | ADR-0014 Risks · SRS-EP-13 floor |

Fixable gaps found during review: none that belong in this file. The alias is pre-existing;
rewriting ADR-0014/0015 or the domain doc was out of scope.

`adlc audit`: 2 orphan code (retired `SRS-IN-13` traces) + 14 orphan SRS including
`SRS-EP-07` / `SRS-EP-08` — **expected** (no `@implements` until W9 code). Not a blocker
for this completeness drop.

## Asks

1. **SM may flip EP-014 / EP-015 / EP-020 from `blocked` → `draft`.** The logic file
   exists. They are **not** `ready` until EP-013 passes.
2. **Do not start W9 implement.** Lock: no `/dev` on any story assuming desktop tree
   authority; no `/dev` on REQ-04 implement before EP-013 passes.
3. Implement stories must apply ops through the EP-07 gesture-commit path (one snapshot,
   one ring entry, one `doc_change` per completed gesture). Recognition/manipulation
   (EP-010/011) write the tree; they do not own it.

## Constraints

- No peer round trip inside an editing gesture.
- Ink latency floor `[SRS-EP-01]` / `[SRS-EP-13]` outranks the document layer.
- Do not fork the wire grammar.

## Out of scope (unchanged)

REQ-08 node-manipulation, multi-directional sync, on-device persistence, new ADRs,
story status `ready` / implement.

## Next

**Type `/sm`** to un-block EP-014 / EP-015 / EP-020 to `draft`. W8 continues:
`/designer` EP-012 ∥ `/dev` EP-013. After EP-013 pass: `/qa` then `/dev` W9+.
