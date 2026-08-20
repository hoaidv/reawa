---
from: pm
to: sm
date: 2026-08-20
iter: iter-005
cc: [architect, designer, qa]
---

# Hand-off: PM → SM — hand-touch field-test lock (REQ-10)

PM does **not** create or edit stories. Product docs reconciled to shipped field-test behavior. Human 2026-08-20 **approved the hand-touch delivery**. Do **not** unpause W3 erase or Device Settings. Do **not** rewrite `.plan/MASTER.md` or the execution board in this lane (parent SM owns those).

## Verdict

**READY-WITH-CONCERNS.** In-place revision of [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) (Hand-touch on canvas). No new REQ. Debug touch-count log is field debug — **not** a requirement.

Epaper PRD **0.12.0-draft**. `adlc prd-check`: **0 FAIL**. Epaper WARNs are pre-existing closed Open Questions (no `owner` on the first bullet line). `reawa` WARNs are out of lock.

### Strengths

- [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) body, acceptance, Success Metrics, and the closed threshold Open Question now lock the same grammar: **20 mm** / **178 du** @ 226 dpi, **≥3** contacts = palm, hand-touch **toggle** default **on**.
- GAP-01 leading **10 mm** Settings tile (`cta.pen_map_open`) left intact — different control.
- ink-box `srs-product.md` / `srs-experience.md` do not contradict (they parent [REQ-05](../../../.docs/modules/epaper/prd.md#device-ink-box) / [REQ-06](../../../.docs/modules/epaper/prd.md#device-manipulation); empty-canvas still deselects). Not edited.

### Concerns (accepted — sibling lanes, not PRD gaps)

- Story AC still say **10 mm** — [STORY-EP-038](../stories/STORY-EP-038.md), [STORY-EP-054](../stories/STORY-EP-054.md). Stories stay `done`. Parent SM replans story text. Dual-ask `/qa`.
- Design package [`.plan/iter-005/design/hand-touch/`](../design/hand-touch/) still paints the 10 mm / 89 du palm threshold and has no HT tile. Dual-ask `/designer` (`btn.hand_touch` + 20 mm copy in UI-EP-06) — sibling Designer owns that write set.
- BDD feature blurb in [hand-touch-one-finger.feature](../../../.docs/modules/epaper/features/ink-box/bdd/hand-touch-one-finger.feature) still says “travel past 10 mm”; scenarios already use 20 mm. `/qa` stitch, not a new REQ.

### Gaps

None for this slice. Architect millimetre bind in [SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) already **20 mm / 178 du**. Module [architecture.md](../../../.docs/modules/epaper/architecture.md) already matches.

## What changed in product

Human field-test lock (2026-08-20) **supersedes** the STORY-EP-054 **10 mm / 89 du** lock.

| Rule | Product truth |
|---|---|
| Palm-rest / empty tap | Euclidean panel travel **≤ 20 mm** (178 du @ 226 dpi). Empty tap still **deselects**. |
| Local one-finger pan | Travel **past 20 mm**. Box / knob / chip hit wins. |
| Palm by count | **≥3** simultaneous capacitive contacts = 0 pan, 0 pinch. |
| Hand-touch toggle | 64 du 1-bit **HT** tile, trailing orientation-top row left of Debug. Default **on**. Off disables canvas pick/move/pan/pinch; chrome taps still work. Pen near or in contact still disables canvas hand-touch. |

## Files changed (this PM turn)

- [`.docs/modules/epaper/prd.md`](../../../.docs/modules/epaper/prd.md) — version **0.12.0-draft**; Success Metrics (20 mm + ≥3 contacts + toggle row); [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) HT affordance + default-on AC + UI states; Open Question closed to 20 mm / 178 du.
- This handoff.

**Not edited:** `srs-product.md` / `srs-experience.md` (no contradiction). Application code. MASTER / execution board / tracks. Architect `architecture.md` / logic millimetre binds. Designer `hand-touch/` package. Story files. W3 erase / [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings).

## Dual-ask (record, do not spawn)

| Persona | Ask |
|---|---|
| `/qa` | Story acceptance still says 10 mm ([STORY-EP-038](../stories/STORY-EP-038.md), [STORY-EP-054](../stories/STORY-EP-054.md)). Parent SM replans story text after this land. BDD Feature blurb still 10 mm. |
| `/designer` | `btn.hand_touch` + **20 mm** copy in UI-EP-06. Sibling Designer sub-agent is doing that. |

## Must replan (existing — SM owns story text)

| Story | Why |
|---|---|
| [STORY-EP-054](../stories/STORY-EP-054.md) | AC + body still lock ≤10 mm / 89 du. Product is **20 mm / 178 du**. Status stays `done`. |
| [STORY-EP-038](../stories/STORY-EP-038.md) | AC still 10 mm (89 du). Product is 20 mm; add ≥3-contact palm + HT toggle default on if missing. Status stays `done`. |
| [STORY-EP-039](../stories/STORY-EP-039.md) | No 10 mm leftover found; confirm two-finger still matches local-only publish. Status stays `done`. |

## Do not

- Create or edit stories in this PM turn (you are SM).
- Unpause W3 erase or Device Settings.
- Mint a REQ for the debug touch-count log.
- Reopen REQ-08 / REQ-15 / CHL-0011 / CHL-0012 / EP-032 / AI / last-writer ADR-0023.
- Treat GAP-01’s leading 10 mm Settings tile as the palm threshold.

## Next

Parent SM: replan story AC to 20 mm / 178 du after Designer lands UI-EP-06. Campaign cursor remains **WAIT human field test**; do not start W3.
