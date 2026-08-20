---
from: architect
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, designer, qa]
---

# Hand-off: Solution Architect → Scrum Master — Device Settings rebind

## Verdict

**READY-WITH-CONCERNS.** Device Settings persist home is the Epaper device. Infini persist/restore is retired in place. Scene graph Keep = `scene.pen_map_editor` only. No new persist SRS id.

## Context

Product lock (PM 2026-08-20): persist **on the Epaper device**, not Infini, not the document. One Settings page ([CHL-0025](../challenges/CHL-0025-pen-map-settings-page.md) adopted). GAP-01 leading tile adopted. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore retired.

## Decision

[ADR-0031](../../../.docs/adr/ADR-0031-device-settings-persist-on-epaper.md) — **Device Settings persist on Epaper** (`status: accepted`). Supersedes [ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md). Authoring-on-tablet and “not document / not SVG” stand. Infini persist/restore does **not**. `pen_capability` may remain optional HID telemetry T→D. **0** `pen_button_map` messages.

No new persist SRS. [SRS-EP-53](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author) is author **and** device persist (parent [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) + [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons)).

## Asks

1. Patch story frontmatter (do not rewrite bodies):
   - [STORY-EP-057](../stories/STORY-EP-057.md) Persist Device Settings on the Epaper device → `parent_srs: [SRS-EP-53]` (keep; now bound)
   - [STORY-EP-058](../stories/STORY-EP-058.md) Implement Device Settings page (Pen buttons) → `parent_srs: [SRS-EP-52]` (keep; sheets dropped)
2. Confirm [STORY-IN-035](../stories/STORY-IN-035.md) stays **cancelled**. Do not implement [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) / [SRS-IN-25](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-25-map-publish-quality).
3. `/qa` BDD for SRS-EP-52 / SRS-EP-53 / SRS-EP-43 before `/dev`. Campaign cursor [STORY-EP-054](../stories/STORY-EP-054.md) is a different write set (`hand-touch/`).
4. Optional PM scrub: Infini / Epaper PRD still say “ADR-0030 still says Infini persist — Architect” — that sentence is now stale. Architect did not edit PRDs.

## SRS ids (id + title + fate)

| ID | Title | Fate |
|---|---|---|
| [SRS-EP-52](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) | Device Settings page (Pen buttons) | **Rebound** — Settings shell, master-detail, inline catalogues, GAP-01 leading 10 mm tile; 0 sheets. Parent REQ-20 + REQ-18 |
| [SRS-EP-53](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author) | On-device pen-button map authoring and persist | **Rebound** — write live map **and** persist on device; 0 persist-up; 0 restore-down |
| [SRS-EP-43](../../../.docs/modules/epaper/features/tool-modes/srs-quality.md#srs-ep-43-barrel-quality) | Barrel dispatch quality | **Rebound** — drop Infini restore bar; add device-restart + 0 Infini copies |
| [SRS-EP-41](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch) | Barrel click vs hold-move dispatch | **Amended** — restore source is device store, not Infini |
| [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) | Pen-button map persist and restore | **Retired** (`superseded-by` SRS-EP-53) |
| [SRS-IN-25](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-25-map-publish-quality) | Pen-button map persist/restore quality | **Retired** (`superseded-by` SRS-EP-43) |
| [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) | Pen-button map settings | **Still retired** — persist note updated |
| `scene.pen_map_editor` | Device Settings · Pen buttons | **Keep** (only Keep scene) |
| `scene.pen_map_click` / `scene.pen_map_hold` | Click / Hold-move sheets | **Retired** in place |

## Review findings

### Strengths

- Persist vs document is MECE: [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) vs [REQ-04](../../../.docs/modules/epaper/prd.md#device-document) / Infini [REQ-02](../../../.docs/modules/infini/prd.md#vector-document).
- Scene graph: Keep = one scene; `present-sheet` withdrawn; GAP-01 disposition **adopted**.
- Infini persist SRS retired in place (ids kept). 0 document-settings fields minted.

### Concerns (accepted)

- **[CHL-0023](../challenges/CHL-0023-epaper-physical-scale.md)** still open: Settings entry tile is **10 mm** (GAP-01); finger floor elsewhere is **64 du** (CHL-0019). Same concern as PM pass. Not blocking this rebind.
- Device-local store **mechanism** unnamed (file vs QSettings). Restart AC is the bar; implementer picks a boring Qt-local store.
- Infini/Epaper PRD leftover “Architect must rebind ADR-0030” — product outcome already locked; sentence is stale until PM scrubs.

### Risks

None that block SM planning. `paths.src` is empty — do **not** `/dev` until `/init` and design + BDD.

## Optional (not blocking EP-054)

[SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) / [SRS-EP-22](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui) already name the **10 mm** palm vs pan rule. No bind this pass. Did not edit `hand-touch/` HTML.

## Constraints

- Do not `/dev` until design done + BDD.
- Do not `/dev` until `/init` (`paths.src` empty).
- Do not invent other Settings master items.
- Do not mint document-settings fields.

## Out of scope

Stories, MASTER, execution board, PRDs, `src/`, `.plan/iter-005/design/hand-touch/`.

## Next

Type `/sm` to patch EP-057 / EP-058 frontmatter if needed and continue the campaign. Designer lane [STORY-EP-054](../stories/STORY-EP-054.md) is independent.
