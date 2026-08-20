---
from: pm
to: architect
date: 2026-08-20
iter: iter-005
cc: [sm, designer, qa]
---

# Hand-off: PM → Architect — Device Settings persist on Epaper

## Verdict

**READY-WITH-CONCERNS.** Product outcomes are MECE, measurable, and prioritised. Architect may decompose / rebind SRS and **supersede** ADRs. PM did not edit `srs-ui` / `srs-logic` / `srs-quality` / `srs-ui-multi-scene` / ADRs / domain.

`adlc prd-check`: **0 FAIL**, 21 WARN. Infini: no findings. Epaper WARNs are pre-existing closed Open Questions (no `owner` on the first bullet line). `reawa` WARNs are out of lock.

## Decision (binding)

Human 2026-08-20:

1. **Device Settings** (example: pen-button map) on Epaper are **saved on the Epaper device**, not to Infini, and not into the document. **No document settings** this campaign.
2. [CHL-0025](../challenges/CHL-0025-pen-map-settings-page.md) **adopted:** one **Settings** page, **master-detail**, first master item **Pen buttons**, Click/Hold-move catalogues **inline**. Drop `present-sheet` / `scene.pen_map_click` / `scene.pen_map_hold`.
3. **GAP-01 adopted:** leading 10 mm stylus-with-barrels tile (`cta.pen_map_open`) is a **sibling of ToolChip**, not a fourth exclusive tool. Written into [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings).
4. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore **retired** (`superseded-by` epaper REQ-20).
5. [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) pan threshold is now product **10 mm** (≤ palm-rest, > local pan). Sufficient for [STORY-EP-054](../stories/STORY-EP-054.md). Do not invent hand-touch inventory.

## Product docs (PM wrote)

| Path | Version | What changed |
|---|---|---|
| [epaper/prd.md](../../../.docs/modules/epaper/prd.md) | 0.11.0-draft | REQ-20 minted; REQ-18 amended; REQ-10 10 mm; persist carve-out |
| [infini/prd.md](../../../.docs/modules/infini/prd.md) | 0.8.0-draft | REQ-05 retired |
| [device-document/srs-product.md](../../../.docs/modules/epaper/features/device-document/srs-product.md) | 0.1.1 | BR-D07 does not cover Device Settings |
| [tool-modes/index.md](../../../.docs/modules/epaper/features/tool-modes/index.md) | — | `parent_req` includes REQ-20 |

REQ ids:

| ID | Title | Fate |
|---|---|---|
| [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) | Device Settings | **Minted** — shell, GAP-01 entry, on-device persist |
| [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons) | Configurable pen barrel-button accelerators | **Amended** — catalogues/dispatch; Pen buttons is REQ-20 detail |
| [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) | Hand-touch on canvas | **Amended** — 10 mm named |
| [REQ-04](../../../.docs/modules/epaper/prd.md#device-document) | On-device working document | **Amended** — document in-memory; Device Settings are not the document |
| Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) | Pen-button map persist (not the editor) | **Retired** |
| Infini [REQ-02](../../../.docs/modules/infini/prd.md#vector-document) | Vector document model, mirror, and persistence | **Amended** — **document** persistence home only |

No REQ deleted. REQ-08 / REQ-15 untouched.

## Architect must-do (rebind — do not wait on PM)

### ADR

1. **Supersede** [ADR-0030](../../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) (Tablet authors pen-button map; Infini persist/restore). Persistence home **moves to the device**. Authoring-on-tablet can stand; **Infini persist/restore split must not**. Keep “not document / not SVG” if still true. PM does not write the ADR.
2. [ADR-0028](../../../.docs/adr/ADR-0028-pen-button-map-settings-channel.md) stays superseded. Settings channel on `:9877` (`pen_capability` / `pen_button_map`) is **optional now** — drop or keep `pen_capability` as HID telemetry only; **do not** restore-on-hello a desktop-stored map.

### Domain

3. [domain/pen-button-map.md](../../../.docs/domain/pen-button-map.md) — persist home = Epaper device; Infini is not persist. Update registry row in [domain/index.md](../../../.docs/domain/index.md).

### Epaper SRS (tool-modes)

4. [SRS-EP-52](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) — Settings shell + master-detail; inline catalogues; drop `present-sheet`; bind GAP-01 leading tile.
5. [srs-ui-multi-scene.md](../../../.docs/modules/epaper/features/tool-modes/srs-ui-multi-scene.md) — Keep = `scene.pen_map_editor` only; retire `scene.pen_map_click` / `scene.pen_map_hold`; GAP-01 disposition = adopted.
6. [SRS-EP-53](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author) — write live map **and persist on device**; drop persist-up to Infini.
7. [SRS-EP-43](../../../.docs/modules/epaper/features/tool-modes/srs-quality.md) — drop Infini restore quality bar; add device-restart persist bar.
8. Optional: new persist SRS id if SRS-EP-53 should stay author-only. Bind [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings).

### Infini SRS (tablet-sync)

9. [SRS-IN-23](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) — **retire in place** (do not delete id). No persist, no restore-on-hello.
10. [SRS-IN-25](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-25-map-publish-quality) — **retire** or invert to “0 Infini copies”.
11. [SRS-IN-24](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md) — already retired editor; confirm still retired.
12. Feature [tablet-sync/index.md](../../../.docs/modules/infini/features/tablet-sync/index.md) `parent_req` still lists REQ-05 — drop or mark retired.

### Other

13. [device-document/srs-data.md](../../../.docs/modules/epaper/features/device-document/srs-data.md) still says live map is “not persisted on device across restart in v1” — **wrong**. Rebind to REQ-20.
14. Do **not** mint document-settings fields on VectorDocument / SVG.
15. Do **not** invent other Settings master items.

## Review findings

### Strengths

- Device Settings vs document persist is MECE ([REQ-20] vs [REQ-04]/[REQ-07]/Infini [REQ-02]).
- Pen-button catalogues stay [REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons); the Settings shell and persist home are [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings).
- CHL-0025 + GAP-01 adopted into product language Designer already painted — no JPEG-as-contract.
- REQ-10 10 mm is now a product number EP-054 can paint without inventing chrome.

### Concerns (accepted)

- **SRS + ADR-0030 + domain** still say Infini persist / `present-sheet`. That is this handoff. Not a PRD gap.
- **CHL-0023** (10 mm tiles vs 64 du) still open. Settings tile is 10 mm in design; product still cites CHL-0019 64 du as the finger floor. Same concern as prior pass.
- Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) retired still carries “0 restore” AC for auditors — SM parks IN-035; do not implement persist.

### Gaps

None that block decomposition. Settings inventory beyond Pen buttons is an explicit Non-Goal.

## Next

Rebind the list above. Then `/sm` replans IN-035 and mints an Epaper persist implement story (draft until SRS + BDD). Campaign continues to `/designer` EP-054. **Do not** `/dev` until design done + BDD.
