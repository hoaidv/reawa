---
from: pm
to: sm
date: 2026-08-20
iter: iter-005
cc: [architect, designer, qa]
---

# Hand-off: PM → SM — replan Device Settings (on-device persist)

PM does **not** create or edit stories. Product docs updated. Replan after this handoff; architect binds SRS (listed in [pm-to-architect](./2026-08-20-pm-to-architect-device-settings.md) — do not wait on PM).

## Verdict

**READY-WITH-CONCERNS.** Human 2026-08-20 is binding. [CHL-0025](../challenges/CHL-0025-pen-map-settings-page.md) **adopted**. GAP-01 **adopted**. Infini persist **retired**.

`adlc prd-check`: **0 FAIL**. Infini: no findings. Epaper WARNs are pre-existing closed Open Questions (no `owner` on the first bullet line). `reawa` WARNs are out of lock.

PRDs: epaper **0.11.0-draft**, infini **0.8.0-draft**.

Do **not** rewrite Master Plan or the execution board in this PM turn — you own those.

## What changed in product

Device Settings live **on the Epaper device**. Settings page is **master-detail**; first (only this package) master item **Pen buttons**; catalogues **inline**. Entry is the leading 10 mm stylus-with-barrels tile. Infini does not store the map. No document settings.

| ID | Title | Fate |
|---|---|---|
| [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) | Device Settings | **Minted** |
| [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) | Configurable pen barrel-button accelerators | **Amended** (editor = Pen buttons detail; persist = REQ-20) |
| [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) | Hand-touch on canvas | **Thickened** (10 mm). Sufficient for EP-054 |
| Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) | Pen-button map persist (not the editor) | **Retired** |

## Must replan (existing)

| Story | Title | What to change |
|---|---|---|
| [STORY-IN-035](../stories/STORY-IN-035.md) | Persist and restore pen-button map (not the editor) | **Park or retire.** Infini no longer stores the map. Do not implement persist/restore. Keep the story id. Parent REQ-05 is retired. |
| [STORY-EP-052](../stories/STORY-EP-052.md) | Barrel click vs hold-move dispatch from catalogue | Keep. Parent stays REQ-18. Drop any implicit depend on Infini persist / IN-035. Dispatch still uses live on-device map. `depends_on` EP-056 is fine. |
| [STORY-EP-056](../stories/STORY-EP-056.md) | Revise pen-button map as Epaper on-device editor | Already `done`. Optionally add `parent_req` REQ-20 after Architect rebind. Do not reopen paint unless Architect/Designer need a Settings-shell AC pass (catalogues + master-detail already in package). |
| [STORY-IN-034](../stories/STORY-IN-034.md) | (historical Infini slate) | Stay historical. Do not retarget persist here. |
| [STORY-EP-054](../stories/STORY-EP-054.md) | Revise hand-touch: palm-rest vs empty local pan | **No inventory replan.** REQ-10 now names ≤10 mm palm-rest vs >10 mm local pan. Campaign continues here after this product lock. Status already `ready`. |

## Add (missing persist story)

| New story (SM mints) | Title (suggested) | What |
|---|---|---|
| **STORY-EP-0xx** (new implement) | Persist Device Settings on the Epaper device | Parent [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) + Architect persist SRS (likely SRS-EP-53 rebind or new id). AC: restart same device → next barrel gesture uses last map; 0 Infini copies; 0 SVG; live map while session down. Freeze until Architect binds + BDD. **Do not** `/dev` until design done + BDD (lock). |

Settings UI implement (if not already covered): a later implement story for SRS-EP-52 Settings shell should `depends_on` EP-056 (package already painted) once Architect drops sheets.

## Architect follow-up (dispatch; PM did not write SRS / ADR)

See [pm-to-architect-device-settings](./2026-08-20-pm-to-architect-device-settings.md). Short list: supersede ADR-0030 persist split; rebind SRS-EP-52/53/43 + scene graph; retire SRS-IN-23/25; fix device-document srs-data “not persisted on device”.

## Dual-ask

- Settings states: Designer already painted [UI-EP-08](../design/pen-button-map/). `/qa` BDD against **one Settings page**, not sheets.
- Hand-touch 10 mm: `/designer` EP-054; `/qa` after that design `done`. Do **not** invent hand-touch UI.

## Do not

- Create or edit stories in this PM turn (you are SM).
- Implement IN-035.
- `/dev` until design done + BDD.
- Bury Device Settings in viewport-follow or hand-touch packages.
- Reopen REQ-08 / REQ-15 / CHL-0011 / CHL-0012 / EP-032 / AI.

## Next

1. `/architect` on the rebind list.
2. You replan IN-035 park/retire + mint Epaper persist implement (draft until SRS + BDD).
3. Campaign cursor: `/designer` [STORY-EP-054](../stories/STORY-EP-054.md) (human glance at UI-EP-08 is done; product lock is in).
