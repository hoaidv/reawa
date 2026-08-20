---
from: architect
to: sm
date: 2026-08-20
iter: iter-005
verdict: READY-WITH-CONCERNS
cc: [pm, qa]
---

# Hand-off: Solution Architect → Scrum Master — hand-touch millimetre bind

Stories, MASTER, execution board, PRD, `hand-touch/` design package, `srs-ui.md`, and application code were **not** edited. No W3. This is an in-place millimetre bind refresh, not a new decomposition.

## Verdict

**READY-WITH-CONCERNS.** Shipped palm/empty-pan millimetres are now the architecture/SRS logic+quality bind. No new Architecture Decision Record. Concerns are sibling leftovers (PRD / story AC / design index), not missing architect binds.

## Bind confirmation (shipped = spec)

Human 2026-08-20 **approved hand-touch**. Millimetre↔du is an in-place refresh of the existing [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) / [SRS-EP-25](../../../.docs/modules/epaper/features/ink-box/srs-quality.md#srs-ep-25-one-finger-quality) palm grammar — not a new costly-to-reverse choice.

| Constant | Shipped | Spec |
|---|---|---|
| Palm / empty-pan travel | `kPalmTravelMm = 20`, `kPalmTravelDu = 178` @ 226 dpi | **20 mm** Euclidean panel travel = **178 du** @ 226 dpi. ≤ = tap / palm-rest (deselect, 0 pan). > = local one-finger pan |
| Min contacts for palm | `kPalmMinContacts = 3` | **≥3** simultaneous contacts = palm (0 pan, 0 pinch) |
| Enable | `handTouchEnabled(penNear, penContact, toggleOn = true)` | `handTouch.enabled = toggle.on and not (pen.near or pen.contact) and contactCount < 3`. Toggle default **on** |
| Trailing chrome index 3 | Hand-touch tile (`viewport_follow.hpp`) | Already in [SRS-EP-22](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui) `btn.hand_touch` (hotfix). Architect did **not** edit `srs-ui.md` |

GAP-01 Settings leading **10 mm** tile (`cta.pen_map_open`) is a **different** control. Left alone in tool-modes / DESIGN.

## ADR

**None written.** 20 mm @ 226 dpi → 178 du replaces 10 mm → 89 du in place. [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md) (Independent cameras + optional one-way viewport follow) still governs cameras. Do not revive [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) (Viewport last-writer token).

## Files changed this pass

| Path | Change |
|---|---|
| `.docs/modules/epaper/architecture.md` | One-finger empty pan past **20 mm** / **178 du** @ 226 dpi (was 10 mm) |
| `.docs/modules/epaper/features/ink-box/srs-logic.md` | [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) no longer says one-finger empty pan is forbidden; points at SRS-EP-21 (20 mm / 178 du). SRS-EP-21 bind already hotfix-correct |
| `.docs/modules/epaper/features/ink-box/bdd/hand-touch-one-finger.feature` | Feature line 10 mm → 20 mm (scenarios already hotfix 20 mm / 178 du) |

## Files read, already bound (no edit)

| Path | Status |
|---|---|
| `.docs/modules/epaper/features/ink-box/srs-quality.md` | SRS-EP-25 already **20 mm** / 178 du |
| `.docs/modules/epaper/features/region-sync/srs-logic.md` | SRS-EP-49 follower local-nav already **past 20 mm** |
| `.docs/modules/epaper/features/region-sync/srs-quality.md` | SRS-EP-51 already empty pan **> 20 mm** |
| `.docs/modules/epaper/features/region-sync/bdd/viewport-follow-epaper.feature` | Already **20 mm / 178 du** (parent was right) |
| `.docs/modules/epaper/features/ink-box/srs-ui.md` | Already 20 mm + `btn.hand_touch` |

## Review (review-design)

### Strengths

- One palm grammar: millimetres in SRS, du at 226 dpi, same numbers as `hand_touch.hpp`.
- Enable predicate is closed and testable (toggle ∧ ¬pen.near ∧ ¬pen.contact ∧ contactCount < 3).
- Architecture remains a view (references SRS-EP-21; defines no new id).

### Concerns (accepted — sibling lanes)

- [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) (Hand-touch on canvas) body is already 20 mm, but the PRD **acceptance table** and one closed-question row still say **10 mm / 89 du**. Architect did not edit `prd.md` (PM).
- [STORY-EP-054](../stories/STORY-EP-054.md) (Revise hand-touch: palm-rest vs empty local pan) AC and body still **10 mm / 89 du**. Story is `done`; SM may note drift — do **not** rewrite MASTER or the execution board from this handoff.
- `.docs/design/index.md` hand-touch row still **≤10 mm / 89 du**. Designer package was out of this lane.

### Risks

None that block the millimetre bind. No new SRS ids. Erase / Device Settings / Infini follow scoring untouched.

## Asks

1. Do **not** open W3 from this bind.
2. Optional PM scrub: PRD table + closed-question **10 mm / 89 du** → **20 mm / 178 du** (same REQ-10; GAP-01 10 mm tile stays).
3. Optional QA: confirm no other Feature descriptions still say 10 mm palm (ink-box one-finger Feature line patched here).
4. Do **not** retarget TRACK-005 or the board for this refresh.

## Constraints

- Execution lock: vertical · stop `verified` · modules epaper, infini · listed features · `out_of_scope: backlog`.
- Forbidden this run: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023.

## Out of scope

Code. New SRS ids. Erase. Device Settings. Infini follow scoring. MASTER. Execution board. `prd.md`. `.plan/iter-005/design/hand-touch/`.
