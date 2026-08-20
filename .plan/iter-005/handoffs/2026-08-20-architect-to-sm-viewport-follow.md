---
from: architect
to: sm
date: 2026-08-20
iter: iter-005
verdict: READY-WITH-CONCERNS
cc: [pm, designer, qa]
---

# Hand-off: Architect → SM — independent cameras + viewport follow

Stories, MASTER, board, design HTML, PRDs, BRD, and `src/` were **not** edited. Replan TRACK-005 from this bind + [pm-to-sm-viewport-follow](./2026-08-20-pm-to-sm-viewport-follow.md).

## Verdict

**READY-WITH-CONCERNS.** Must follow + independent cameras have testable SRS, measurable quality, and a named ADR. Last-writer is superseded. Designer can pick up **new** follow-toggle packages from `srs-ui` skeletons. Concerns below are PM/QA/package-delta follow-ups, not missing Must specs.

Follower local-nav → follow **off**: **accepted** (no `CHL-*`). Restoring last-writer steal would fight cameras.

Pan threshold: **10 mm** Euclidean panel travel (**89 du** @ 226 dpi). At/below = palm-rest / tap. Past = local pan. Box / knob / chip hit wins.

Infini→Infini follow is **not** specified (Non-Goal). Document channel stays one-way. Follow is **not** a ToolChip exclusive tool.

## New / rebound SRS

| ID | Title | Parent REQ | Path |
|---|---|---|---|
| [SRS-EP-49](../../../.docs/modules/epaper/features/region-sync/srs-logic.md#srs-ep-49-viewport-follow) | Viewport-follow Infini | [REQ-19](../../../.docs/modules/epaper/prd.md#viewport-follow) | `epaper/region-sync/srs-logic.md` |
| [SRS-EP-50](../../../.docs/modules/epaper/features/region-sync/srs-ui.md#srs-ep-50-follow-toggle) | Viewport-follow Infini toggle | REQ-19 | `epaper/region-sync/srs-ui.md` (`needs_design: yes`) |
| [SRS-EP-51](../../../.docs/modules/epaper/features/region-sync/srs-quality.md#srs-ep-51-follow-quality) | Viewport-follow Infini quality | REQ-19 | `epaper/region-sync/srs-quality.md` |
| [SRS-IN-26](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-26-viewport-follow) | Viewport-follow Epaper | Infini [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) | `infini/tablet-sync/srs-logic.md` |
| [SRS-IN-27](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-27-follow-toggle) | Viewport-follow Epaper toggle | Infini REQ-06 | `infini/tablet-sync/srs-ui.md` (`needs_design: yes`) |
| [SRS-IN-28](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-28-follow-quality) | Viewport-follow Epaper quality | Infini REQ-06 | `infini/tablet-sync/srs-quality.md` |

**Rebind (link, not parents of follow):**

| ID | What changed |
|---|---|
| [SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md) | Apply Infini viewport **only while** Epaper follow on |
| [SRS-EP-24](../../../.docs/modules/epaper/features/region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) | Local Must; publish **only if** Infini follow on; follower-nav turns Epaper follow off |
| [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) | Empty pan **> 10 mm**; ≤10 mm palm; hit wins; publish only if Infini following |
| [SRS-IN-20](../../../.docs/modules/infini/features/infinity-canvas/srs-logic.md#srs-in-20-follow-viewport) | Apply tablet viewport **only while** Infini follow on. Parent stays Infini [REQ-01](../../../.docs/modules/infini/prd.md#infinity-canvas) |
| [SRS-IN-21](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-21-viewport-token) | Emit/apply gates (token withdrawn). Parent stays Infini [REQ-03](../../../.docs/modules/infini/prd.md#tablet-sync) |
| [SRS-IN-22](../../../.docs/modules/infini/features/infinity-canvas/srs-quality.md#srs-in-22-follow-quality) | Apply-while-following quality; not last-writer |

Do **not** parent REQ-19 / Infini REQ-06 on EP-02 / EP-24 / IN-20 / IN-21.

## ADR

| ID | Title | Status |
|---|---|---|
| [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md) | Independent cameras + optional one-way viewport follow | **accepted** (supersedes ADR-0023; amends ADR-0009 / ADR-0015 §5/§7) |
| [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) | Viewport last-writer token | **superseded** — do not implement |

Anatomy: [domain/viewport-follow](../../../.docs/domain/viewport-follow.md). Glossary “Viewport token” retired.

## Story retarget table (SM — do not apply in this run)

Highest existing: EP-052, IN-035. Suggested next ids below; SM owns allocation.

| Story | Action | Proposed parent_srs | Proposed parent_req |
|---|---|---|---|
| **New** design `viewport-follow-epaper/` (e.g. EP-053) | **Create.** Icon toggle. Dual-ask designer + QA. **Not** a ToolChip tile. **Do not** bury in EP-037 | SRS-EP-50 (logic SRS-EP-49, quality SRS-EP-51) | REQ-19 |
| **New** design `viewport-follow-infini/` (e.g. IN-036) | **Create.** Same states, follow **Epaper**. Not IN-034 | SRS-IN-27 (logic SRS-IN-26, quality SRS-IN-28) | Infini REQ-06 |
| **New** implement after those design stories | Toggle + session enum + mutual exclusion + disconnect | SRS-EP-49 / SRS-EP-51 and SRS-IN-26 / SRS-IN-28 | REQ-19 / Infini REQ-06 |
| [STORY-EP-037](../stories/STORY-EP-037.md) | Status `done` — **do not** add follow buttons. **Revise package or add follow-on design on the same `hand-touch/` package:** `hand.one_finger_empty_palm` + `hand.one_finger_empty_pan`; two-finger scenes must not imply always-on Infini match | Keep SRS-EP-21, SRS-EP-22, SRS-EP-23, SRS-EP-24 | REQ-10 |
| [STORY-EP-038](../stories/STORY-EP-038.md) | Replan AC: ≤**10 mm** = 0 pan; **>10 mm** = local pan; box/knob/chip wins. Drop “empty = 0 pan” | SRS-EP-21, SRS-EP-23, SRS-EP-25 | REQ-10 |
| [STORY-EP-039](../stories/STORY-EP-039.md) | **Local** two-finger Must; publish **only if Infini follow on**. Drop BRD-07 ship-gate language. Drop last-writer AC. Follower-nav turns Epaper follow off | SRS-EP-24, SRS-EP-26 (link SRS-EP-49, do not parent REQ-19 here) | REQ-10 |
| [STORY-IN-033](../stories/STORY-IN-033.md) | Apply tablet viewport **only while Infini follow is on**. Drop last-writer / “tablet still applies Infini after gesture.” Independent default | SRS-IN-20, SRS-IN-21, SRS-IN-22, SRS-IN-26 | Infini REQ-03 + REQ-06 |

Freeze any AC that still says last-writer steal until this bind is the parent.

## Review (review-design)

### Strengths

- Mapping ([SRS-EP-02] / Infini [REQ-02] analog IN-20), gestures ([SRS-EP-21]/[SRS-EP-24]), and follow ([SRS-EP-49]/[SRS-IN-26]) stay MECE.
- `direction` enum makes dual-on count **0** by construction; document types stay auditable (`viewport_follow` is session, not `doc_*`).
- One-finger palm vs pan is one grammar (10 mm + hit-test), QA-testable without millimetres in the PRD.
- Follow chrome is a separate `needs_design` surface, not a fourth exclusive tool.

### Concerns (accepted unless noted)

- [device-document srs-product](../../../.docs/modules/epaper/features/device-document/srs-product.md) **BR-D08** still says downward traffic is always load + viewport. **PM owns** `srs-product`; architect did not edit. Ask PM to amend “viewport only along follow.”
- BDD `one-way-sync.feature` still “viewport continues.” **QA** retags after SM replans.
- [STORY-EP-037](../stories/STORY-EP-037.md) is `done` with empty = no-op. Package delta is required before EP-038 can match SRS.
- Shipped Qt/Electron still always-on / last-writer. Spec lag is expected; stories must not AC the old token.

### Risks

None that block design stories. Sensitivity: **0 dual-on** vs **toggle latency** — p95 ≤300 ms peer-off is the comfort bar ([SRS-EP-51] / [SRS-IN-28]). Trade-off: independent cameras vs one shared picture — follow is opt-in; follower-nav dropping follow is how they do not fight.

## Constraints (lock)

Vertical, stop `verified`, bounded, wip 2, modules epaper+infini. **Forbidden:** REQ-15 tables, REQ-08, CHL-0011 / CHL-0012, EP-032, AI. CHL-0024 knobs stay.

## Next

`/sm` replan TRACK-005: two new follow design stories + EP-037 package delta + EP-038 / EP-039 / IN-033 AC. Then `/designer` on the new packages (dual-ask QA).
