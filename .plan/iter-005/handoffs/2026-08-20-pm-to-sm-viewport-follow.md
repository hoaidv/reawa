---
from: pm
to: sm
date: 2026-08-20
iter: iter-005
cc: [architect, designer, qa]
---

# Hand-off: PM → SM — replan viewport follow (do not create stories here)

PM does **not** create or edit stories. Replan after architect binds SRS.

## What changed in product

Independent cameras by default. Optional mutually exclusive follow. Two-finger **local** pan is Must (BRD-07 ship gate **lifted**). One-finger empty canvas **pans** (threshold vs palm). Always-on Infini→tablet drive is obsolete.

PRDs: epaper **0.9.0-draft**, infini **0.6.0-draft**.

## New `kind: design` surfaces (new stories — do **not** bury in EP-037)

| Surface | Parent REQ | Notes |
|---|---|---|
| Epaper viewport-follow **icon toggle** | [REQ-19](../../../.docs/modules/epaper/prd.md#viewport-follow) | Off / following Infini / peer-following-you / connection lost → off / reconnect stays off. **Not** a ToolChip exclusive tile. New package (e.g. `viewport-follow-epaper/`). Dual-ask designer + QA. |
| Infini viewport-follow **icon toggle** | [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) | Same states, follow **Epaper**. New package (e.g. `viewport-follow-infini/`). Not IN-034 pen-button-map. Dual-ask designer + QA. |

Likely **new implement** stories after those design stories (architect binds SRS). Do not fold follow chrome into [STORY-EP-037](../stories/STORY-EP-037.md).

## Must replan (existing)

| Story | Why |
|---|---|
| [STORY-EP-039](../stories/STORY-EP-039.md) | No longer “always publish viewport” / Infini always matches. **Local** two-finger pan/pinch is Must; publish **only if Infini follow is on**. Drop BRD-07 ship-gate language. Drop last-writer AC. |
| [STORY-IN-033](../stories/STORY-IN-033.md) | No longer “always follow tablet-published viewport.” Apply tablet viewport **only while Infini follow is on**. Drop last-writer / “tablet still applies Infini after gesture.” Retarget parent to [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) + [REQ-03](../../../.docs/modules/infini/prd.md#tablet-sync) once architect binds. |
| [STORY-EP-038](../stories/STORY-EP-038.md) | AC today: one-finger empty = **0 pan**. Product is now: below threshold = palm no-op; **past threshold = local pan**; box hit wins. Replan AC + scenes. |
| [STORY-EP-037](../stories/STORY-EP-037.md) package | Status `done`, but **must revise** (or add a follow-on design story on the same package): `hand-touch-one-finger-empty` is a **no-op** scene; need **palm-rest no-op** and **empty local pan**. Two-finger scenes must not imply always-on Infini match. **Do not** add follow-toggle buttons to this package. |

## Do not

- Create stories in this PM turn (you are SM).
- Schedule Infini→Infini follow.
- Reopen REQ-08 / REQ-15 / CHL-0011 / CHL-0012 / EP-032 / AI.
- Treat [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) last-writer as the UX — architect supersedes; freeze implement stories that AC last-writer until that bind.

## Next

Wait for `/architect` SRS + ADR supersession, then replan TRACK-005 board (new design stories + EP-037 package delta + EP-038 / EP-039 / IN-033 AC).
