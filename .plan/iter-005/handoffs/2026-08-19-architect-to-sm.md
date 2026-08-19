---
from: architect
to: sm
date: 2026-08-19
iter: iter-005
verdict: READY-WITH-CONCERNS
---

# Hand-off: Architect → SM — TRACK-005 W0 SRS bind

Wave 0 bind for **Hand-on-paper**. New `[SRS-EP-21]`…`[SRS-EP-48]` and `[SRS-IN-20]`…`[SRS-IN-25]` are `lifecycle: active`. Stories were **not** retargeted (SM owns that). Design packages were **not** started.

## Verdict

**READY-WITH-CONCERNS** — Must requirements have testable SRS + measurable quality + named ADRs. Designer can pick up EP-037 ∥ IN-034 from the new `srs-ui` skeletons. Concerns below are PM/analyst follow-ups, not missing Must specs.

REQ-17 (Should) **was specified** (not skipped).

## New SRS ids

| ID | Title | Parent REQ | Path |
|---|---|---|---|
| [SRS-EP-21](../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) | One-finger pick and move | [REQ-10](../../.docs/modules/epaper/prd.md#hand-touch) | `epaper/ink-box/srs-logic.md` |
| [SRS-EP-22](../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui) | Hand-touch chrome and hit rules | REQ-10 | `epaper/ink-box/srs-ui.md` |
| [SRS-EP-23](../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-23-finger-tool-switch) | Finger exclusive-tool switch | REQ-10 | `epaper/tool-modes/srs-logic.md` |
| [SRS-EP-24](../../.docs/modules/epaper/features/region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) | Two-finger pan/zoom and viewport publish | REQ-10 | `epaper/region-sync/srs-logic.md` |
| [SRS-EP-25](../../.docs/modules/epaper/features/ink-box/srs-quality.md#srs-ep-25-one-finger-quality) | One-finger hand-touch quality | REQ-10 | `epaper/ink-box/srs-quality.md` |
| [SRS-EP-26](../../.docs/modules/epaper/features/region-sync/srs-quality.md#srs-ep-26-two-finger-quality) | Two-finger map-apply quality | REQ-10 | `epaper/region-sync/srs-quality.md` |
| [SRS-EP-27](../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md#srs-ep-27-eraser-nib) | Hardware eraser-nib stroke-erase | [REQ-11](../../.docs/modules/epaper/prd.md#erase) | `epaper/local-pen-ink/srs-logic.md` |
| [SRS-EP-28](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-28-selection-erase) | Selection-erase and undo | REQ-11 | `epaper/device-document/srs-logic.md` |
| [SRS-EP-29](../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-29-erase-ui) | Erase chrome | REQ-11 | `epaper/tool-modes/srs-ui.md` |
| [SRS-EP-30](../../.docs/modules/epaper/features/local-pen-ink/srs-quality.md#srs-ep-30-erase-quality) | Erase latency and correctness | REQ-11 | `epaper/local-pen-ink/srs-quality.md` |
| [SRS-EP-31](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) | In-document clipboard ops | [REQ-12](../../.docs/modules/epaper/prd.md#clipboard) | `epaper/device-document/srs-logic.md` |
| [SRS-EP-32](../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-32-clipboard-ui) | Clipboard affordances | REQ-12 | `epaper/ink-box/srs-ui.md` |
| [SRS-EP-33](../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-33-clipboard-quality) | Clipboard fidelity | REQ-12 | `epaper/device-document/srs-quality.md` |
| [SRS-EP-34](../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-34-end-styles) | Per-end endpoint styles | [REQ-13](../../.docs/modules/epaper/prd.md#connector-ends) | `epaper/connector-ink/srs-logic.md` |
| [SRS-EP-35](../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-35-endpoint-ink) | Endpoint-ink membership | REQ-13 | `epaper/connector-ink/srs-logic.md` |
| [SRS-EP-36](../../.docs/modules/epaper/features/connector-ink/srs-ui.md#srs-ep-36-endpoint-toolbar) | Endpoint style toolbar | REQ-13 | `epaper/connector-ink/srs-ui.md` |
| [SRS-EP-37](../../.docs/modules/epaper/features/connector-ink/srs-quality.md#srs-ep-37-endpoint-quality) | Endpoint style and endpoint-ink quality | REQ-13 | `epaper/connector-ink/srs-quality.md` |
| [SRS-EP-38](../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-38-attachment-t) | Mid-attachment parameter t | [REQ-14](../../.docs/modules/epaper/prd.md#connector-attachments) | `epaper/connector-ink/srs-logic.md` |
| [SRS-EP-39](../../.docs/modules/epaper/features/connector-ink/srs-ui.md#srs-ep-39-attachment-ui) | Attachment bind chrome | REQ-14 | `epaper/connector-ink/srs-ui.md` |
| [SRS-EP-40](../../.docs/modules/epaper/features/connector-ink/srs-quality.md#srs-ep-40-attachment-quality) | Attachment warp-follow quality | REQ-14 | `epaper/connector-ink/srs-quality.md` |
| [SRS-EP-41](../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch) | Barrel click vs hold-move dispatch | [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) | `epaper/tool-modes/srs-logic.md` |
| [SRS-EP-42](../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool) | Chip mirrors temporary tool | REQ-18 | `epaper/tool-modes/srs-ui.md` |
| [SRS-EP-43](../../.docs/modules/epaper/features/tool-modes/srs-quality.md#srs-ep-43-barrel-quality) | Barrel dispatch quality | REQ-18 | `epaper/tool-modes/srs-quality.md` |
| [SRS-EP-44](../../.docs/modules/epaper/features/tool-modes/srs-logic.md#srs-ep-44-manual-create-routing) | Manual create routing | [REQ-17](../../.docs/modules/epaper/prd.md#manual-create) | `epaper/tool-modes/srs-logic.md` |
| [SRS-EP-45](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-45-manual-insert) | Manual insert Frame and Primitive | REQ-17 | `epaper/device-document/srs-logic.md` |
| [SRS-EP-46](../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-46-manual-connector) | Manual connector and attach | REQ-17 | `epaper/connector-ink/srs-logic.md` |
| [SRS-EP-47](../../.docs/modules/epaper/features/tool-modes/srs-ui.md#srs-ep-47-manual-create-ui) | Manual create chrome | REQ-17 | `epaper/tool-modes/srs-ui.md` |
| [SRS-EP-48](../../.docs/modules/epaper/features/tool-modes/srs-quality.md#srs-ep-48-manual-create-quality) | Manual create quality | REQ-17 | `epaper/tool-modes/srs-quality.md` |
| [SRS-IN-20](../../.docs/modules/infini/features/infinity-canvas/srs-logic.md#srs-in-20-follow-viewport) | Follow tablet-published viewport | Epaper REQ-10 | `infini/infinity-canvas/srs-logic.md` |
| [SRS-IN-21](../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-21-viewport-token) | Viewport last-writer session rules | Epaper REQ-10 | `infini/tablet-sync/srs-logic.md` |
| [SRS-IN-22](../../.docs/modules/infini/features/infinity-canvas/srs-quality.md#srs-in-22-follow-quality) | Tablet-follow viewport quality | Epaper REQ-10 | `infini/infinity-canvas/srs-quality.md` |
| [SRS-IN-23](../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish) | Pen-button map persist and settings publish | Infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) | `infini/tablet-sync/srs-logic.md` |
| [SRS-IN-24](../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) | Pen-button map settings | Infini REQ-05 | `infini/tablet-sync/srs-ui.md` |
| [SRS-IN-25](../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-25-map-publish-quality) | Pen-button map publish quality | Infini REQ-05 | `infini/tablet-sync/srs-quality.md` |

Infini [SRS-IN-05](../../.docs/modules/infini/features/vector-document/srs-ui.md) Document chrome (open/save) is **not** a parent of REQ-05 / IN-034.

## New ADRs

| ID | Title | Path | Status |
|---|---|---|---|
| [ADR-0023](../../.docs/adr/ADR-0023-viewport-last-writer.md) | Viewport last-writer token (tablet vs Infini) | `.docs/adr/ADR-0023-viewport-last-writer.md` | proposed |
| [ADR-0024](../../.docs/adr/ADR-0024-in-document-clipboard.md) | In-document clipboard ops (one slot) | `.docs/adr/ADR-0024-in-document-clipboard.md` | proposed |
| [ADR-0025](../../.docs/adr/ADR-0025-barrel-vs-eraser-nib.md) | Barrel-button channel vs hardware eraser nib | `.docs/adr/ADR-0025-barrel-vs-eraser-nib.md` | proposed |
| [ADR-0026](../../.docs/adr/ADR-0026-endpoint-ink-membership.md) | Endpoint-ink membership (end vs spine vs empty) | `.docs/adr/ADR-0026-endpoint-ink-membership.md` | proposed |
| [ADR-0027](../../.docs/adr/ADR-0027-attachment-t-rest-spine.md) | Attachment parameter t on connector rest spine | `.docs/adr/ADR-0027-attachment-t-rest-spine.md` | **accepted** (forced by ADR-0020 I1) |
| [ADR-0028](../../.docs/adr/ADR-0028-pen-button-map-settings-channel.md) | Pen-button map publish is a settings channel | `.docs/adr/ADR-0028-pen-button-map-settings-channel.md` | **accepted** (forced by ADR-0015 + REQ-05) |

3 and 6 were **not** merged — HID routing ≠ settings transport.

## Story retarget table (SM — do not apply in this run)

| Story | Current parent_srs (leave as-is in files) | Proposed parent_srs |
|---|---|---|
| STORY-EP-037 | SRS-EP-11, SRS-EP-12, SRS-EP-04 | SRS-EP-21, SRS-EP-22, SRS-EP-23, SRS-EP-24 |
| STORY-EP-038 | SRS-EP-11, SRS-EP-04 | SRS-EP-21, SRS-EP-23, SRS-EP-25 |
| STORY-EP-039 | SRS-EP-02, SRS-EP-08 | SRS-EP-24, SRS-EP-26 |
| STORY-IN-033 | SRS-IN-07, SRS-IN-02 | SRS-IN-20, SRS-IN-21, SRS-IN-22 |
| STORY-EP-040 | SRS-EP-05, SRS-EP-12 | SRS-EP-29, SRS-EP-27, SRS-EP-28 |
| STORY-EP-041 | SRS-EP-01, SRS-EP-07 | SRS-EP-27, SRS-EP-30 |
| STORY-EP-042 | SRS-EP-11, SRS-EP-07 | SRS-EP-28, SRS-EP-30 |
| STORY-EP-043 | SRS-EP-12 | SRS-EP-32, SRS-EP-31 |
| STORY-EP-044 | SRS-EP-07 | SRS-EP-31, SRS-EP-33 |
| STORY-EP-045 | SRS-EP-19 | SRS-EP-36, SRS-EP-34 |
| STORY-EP-046 | SRS-EP-19, SRS-EP-18 | SRS-EP-34, SRS-EP-37 |
| STORY-EP-047 | SRS-EP-17, SRS-EP-18 | SRS-EP-35, SRS-EP-37 |
| STORY-EP-048 | SRS-EP-19 | SRS-EP-39, SRS-EP-38 |
| STORY-EP-049 | SRS-EP-18 | SRS-EP-38, SRS-EP-40 |
| STORY-EP-050 | SRS-EP-05 | SRS-EP-47, SRS-EP-44 |
| STORY-EP-051 | SRS-EP-07 | SRS-EP-45, SRS-EP-46, SRS-EP-48 |
| STORY-EP-052 | SRS-EP-04 | SRS-EP-41, SRS-EP-42, SRS-EP-43 |
| STORY-IN-034 | SRS-IN-05 | **SRS-IN-24, SRS-IN-23** (not SRS-IN-05) |
| STORY-IN-035 | SRS-IN-07 | SRS-IN-23, SRS-IN-25 |

## Review findings (`review-design.md`)

### Strength

- New ids; old sections not overloaded as parents (SRS-EP-11/04/02, SRS-IN-05 linked only).
- Quality bars from PRD are numeric (300 ms / 0 px / ≥5 Hz / 100 ms / 50 ms / 0 document messages).
- Closed catalogues for Click, Hold-move, endpoint styles, erase paths, primitives; 64 du finger rule stated.
- ADR-0020 I1 preserved (attachment `t` on rest spine; no rebake).
- Settings vs document split is type-auditable (ADR-0028).

### Concern

- **PM `srs-experience` / `srs-product` not thickened** for these REQs (architect did not author PM files). Designer contracts exist; journeys remain PRD “UI states” lists. Flag for `/pm` after W1 if Designer needs more.
- **PRD campaign comments** still say “Not in the current lock” / “iter-005 draft” on REQ-11…18 and Infini REQ-05. **Master Plan lock wins** — specify done. PM should clean those comments (do not silently rewrite PRD here).
- **[CHL-0022](../challenges/CHL-0022-shipped-no-device-pan.md)** — shipped SRS-EP-04 / SRS-EP-11 / SRS-EP-19 clauses still on disk; implement against **new** ids until PM adopts.
- **BRD-07** still defers on-device pan: EP-039 / IN-033 stay gated even though SRS-EP-24 exists.
- Path A sample-erase v1 may publish `restore_snapshot` until an `erase_samples` wire op exists (stated in SRS-EP-28).
- ADR-0023/24/25/26 remain **proposed** until PM accepts.
- No trio session (lock: SM spawns Designer next).

### Risk

- **Token steal vs 0 fighting bursts** (ADR-0023 sensitivity). Mitigated by exclusive owner; untested on hardware.
- **Endpoint-ink false positives** vs REQ-09 ≤2% ship gate — tight `s` windows; retune only via CHL in du.
- **Two geometry implementations** still the fidelity risk (fixtures must grow for terminals/attachments/`duplicate_subtree`).

## PM follow-up (not done this wave)

1. Adopt CHL-0022 clause amendments.
2. Clean PRD campaign comments to match MASTER lock.
3. Optional: thicken `srs-experience` / `srs-product` for hand-touch, erase, clipboard, ends, attach, barrel, manual create.
4. Accept proposed ADRs 0023–0026.

## Out of lock (not specified)

REQ-15 tables · REQ-08 · CHL-0011 · CHL-0012 · STORY-EP-032.

## Next

SM retargets `parent_srs` from the table. Then **`/designer` EP-037 ∥ IN-034**.

## Paths changed

- `.docs/adr/ADR-0023` … `ADR-0028`
- `.docs/domain/vector-document.md`, `pen-button-map.md`, `index.md`
- `.docs/glossary.md` (new)
- `.docs/modules/epaper/features/{ink-box,tool-modes,connector-ink,region-sync,local-pen-ink,device-document}/**`
- `.docs/modules/epaper/architecture.md`
- `.docs/modules/infini/features/{infinity-canvas,tablet-sync,vector-document/srs-data.md}`
- `.docs/modules/infini/architecture.md`
- `.plan/iter-005/challenges/CHL-0022-shipped-no-device-pan.md`
- this handoff
