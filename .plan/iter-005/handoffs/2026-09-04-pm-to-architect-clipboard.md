---
from: pm
to: architect
date: 2026-09-04
iter: iter-005
---

# Hand-off: Product Manager → Architect

## Context

Human started [REQ-12](../../../.docs/modules/epaper/prd.md#clipboard) (Copy, cut, and paste on the device). Waiting `ToolAction` stubs exist; prior [ADR-0024](../../../.docs/adr/ADR-0024-in-document-clipboard.md) (+24 offset, slot on DeviceDocument, Infini `duplicate_subtree` this track) is **wrong** for the lock-in.

**Coherent product doc:** [clipboard srs-product](../../../.docs/modules/epaper/features/clipboard/srs-product.md) ([SRS-EP-73](../../../.docs/modules/epaper/features/clipboard/srs-product.md#srs-ep-73-clipboard-product)). Index: [clipboard/index.md](../../../.docs/modules/epaper/features/clipboard/index.md). PRD REQ-12 updated (`0.15.0-draft`, **Needs design: no**).

## Review verdict

**READY**

| Class | Finding |
|---|---|
| Strength | Outcome unchanged; paste origin = press; hold vs Selected chrome split; local-only wire; skip design story |
| Concern | Empty-clipboard hold still **deselects** with no chrome (BR-C08) — accepted with human |
| Gap | none blocking bind |

`prd-check` run with this handoff.

## Asks

1. Rewrite [SRS-EP-31](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) / [SRS-EP-32](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-32-clipboard-ui) / [SRS-EP-33](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-33-clipboard-quality) against SRS-EP-73. Keep IDs.
2. Hold-still (500 ms / ≤1 mm panel) in [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation) + tool-system routing. Primary tap-select. Fixes tap-nudge.
3. Supersede ADR-0024 with a new ADR (singleton, 0 wire, hold toolbar).
4. BDD `clipboard.feature` before code.
5. Cancel [STORY-EP-043](../stories/STORY-EP-043.md). Ready [STORY-EP-044](../stories/STORY-EP-044.md) without a design `depends_on`.

## Constraints

- Vertical TRACK-005. Do not mix EP-070…072, Device Settings, TRACK-006.
- Infini `applyOp` / closed transmit list **out**.
- Do not put copy/cut/paste verbs on `DocContext`. Actions + clipboard singleton + low-level edits only.
- Chrome frozen in SRS — no Designer package.

## Out of scope

- OS pasteboard, ToolChip, barrel, Infini mirror apply, design story EP-043.
