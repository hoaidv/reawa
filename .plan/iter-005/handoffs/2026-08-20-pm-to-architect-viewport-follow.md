---
from: pm
to: architect
date: 2026-08-20
iter: iter-005
cc: [sm, designer, qa, analyst]
---

# Hand-off: PM → Architect — independent viewports + optional follow

## Verdict

**READY-WITH-CONCERNS.** Product outcomes are MECE, measurable, and prioritised. Architect may decompose SRS / supersede ADRs. Do **not** wait on BRD-07 file save — human 2026-08-20 is the source.

`adlc prd-check`: **0 FAIL**. WARNs are pre-existing closed Open Questions (no `owner` on the first bullet line) plus out-of-lock `reawa` PRD. No new FAIL on epaper / infini.

## Decision (binding)

Human 2026-08-20:

1. Tablet **can** change its viewport. Epaper and Infini cameras are **independent by default**. Always-on Infini→tablet viewport drive is **obsolete**.
2. Optional **follow**, mutually exclusive, off on disconnect. Infini toggle = follow Epaper (Epaper → Infini). Epaper toggle = follow Infini (Infini → Epaper). Exactly one direction at a time when connected; both off allowed. Reconnect does **not** restore follow.
3. Affordance: **icon toggle button** on **both** peers. **Not** a ToolChip exclusive hand-tool tile. `needs_design: yes`.
4. Infini remains navigator + persistence home. Document channel stays **one-way**. No hand-tool chip tile.
5. One-finger empty canvas: **local pan** (not a no-op). Reconciled with palm-rest via a documented movement threshold; box/knob/chip hit wins over pan.

## Product docs (PM wrote)

| Path | Version | What changed |
|---|---|---|
| [epaper/prd.md](../../../.docs/modules/epaper/prd.md) | 0.9.0-draft | Follow-gated mapping; local pan Must; new follow toggle |
| [infini/prd.md](../../../.docs/modules/infini/prd.md) | 0.6.0-draft | Independent cameras; follow-gated session; new follow toggle |

REQ ids — **amended** (same id, lifecycle still `active`):

| ID | Title | Path |
|---|---|---|
| [REQ-02](../../../.docs/modules/epaper/prd.md#region-sync) | Drawing-region mapping from Infini | epaper — apply Infini viewport **only while Epaper follow is on**; default local camera |
| [REQ-03](../../../.docs/modules/epaper/prd.md#tool-modes) | On-device tool modes | epaper — follow is **not** a ToolChip tile (one-line cross-link) |
| [REQ-07](../../../.docs/modules/epaper/prd.md#one-way-sync) | One-way document sync | epaper — viewport down is follow-gated; document channel still one-way |
| [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) | Hand-touch on canvas | epaper — two-finger **local** Must (BRD-07 ship gate lifted); one-finger empty = local pan vs palm threshold; publish **only if Infini is following**; CHL-0024 knobs stay |
| [REQ-01](../../../.docs/modules/infini/prd.md#infinity-canvas) | Infinity canvas navigation | infini — Infini still pans **its** canvas; reaches tablet only while Epaper follow is on |
| [REQ-03](../../../.docs/modules/infini/prd.md#tablet-sync) | Tablet session — one-way sync contract | infini — stop always-on `viewport` down; load stays; viewport only along active follow |

REQ ids — **added**:

| ID | Title | Path |
|---|---|---|
| [REQ-19](../../../.docs/modules/epaper/prd.md#viewport-follow) | Viewport-follow Infini | epaper — icon toggle; off / following Infini / lost → off / mutual exclusion |
| [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) | Viewport-follow Epaper | infini — icon toggle; follow Epaper; same exclusion + off on disconnect |

No REQ deleted. REQ-16 remains retired. REQ-08 / REQ-15 untouched.

## Asks

1. **Supersede** [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) (viewport last-writer token). It is **not** the product model. Write a follow / token-optional ADR. Do not implement last-writer as the UX.
2. Rebind SRS that assume always-on Infini drive or last-writer: [SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md), [SRS-EP-24](../../../.docs/modules/epaper/features/region-sync/srs-logic.md), [SRS-IN-20](../../../.docs/modules/infini/features/infinity-canvas/srs-logic.md), [SRS-IN-21](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md). Bind **new** SRS for [REQ-19](../../../.docs/modules/epaper/prd.md#viewport-follow) and [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) (toggle states + mutual exclusion). PM does not write SRS.
3. Bind the **documented pan threshold** for one-finger empty pan ([REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) open question). Product rule is already: at/below = palm-rest no-op; past = local pan; box/knob/chip wins. Do not specify that number in the PRD.
4. Do **not** specify Infini→Infini follow as Must. Do **not** change the document channel to two-way.

## Review findings

### Strengths

- Follow is a **choice**, mapping is still [REQ-02](../../../.docs/modules/epaper/prd.md#region-sync) (pen-sample map), gestures stay [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) — MECE.
- Empty-canvas pan vs palm-rest vs box pick is one grammar (threshold + hit-test), not two contradictory ACs.
- Document one-way and Infini-as-navigator are explicit Non-Goals / kept Musts.
- Dual-ask on both follow toggles (`needs_design: yes` + states matrix).

### Concerns (accepted)

- **BRD-07 file** still says Infini drives the RM region and defers on-device pan. Analyst is amending in parallel. This PRD is the product source until BRD catches up.
- **SRS + ADR-0023 + architecture.md + glossary “Viewport token”** still describe last-writer. Architect must supersede; PM did not edit those.
- **Pan threshold number** is not in the PRD (architect binds). Accepted: product rule is testable without millimetres in the REQ.
- **Follower local-nav turns follow off** is PM completion of “not last-writer.” Human did not say this sentence; without it, a following peer who pans fights the leader. Flag if you disagree — do not silently restore last-writer.

### Gaps

None that block decomposition. Follow UI placement (vs ToolChip) is a design story, not an SRS-block.

## Constraints

- No wire fields in product docs. No ADR text from PM.
- Lock: no REQ-15 tables, no REQ-08 work, no CHL-0011 / CHL-0012, no EP-032, no AI.
- CHL-0024 finger resize knobs stay adopted.

## Out of scope

- `.docs/brd.md` (analyst). SRS, ADRs, stories, design HTML, `src/`.
- Infini→Infini follow. Two-way document sync.

## Next

`/architect` decompose + supersede ADR-0023. Then `/sm` using [pm-to-sm-viewport-follow](./2026-08-20-pm-to-sm-viewport-follow.md).
