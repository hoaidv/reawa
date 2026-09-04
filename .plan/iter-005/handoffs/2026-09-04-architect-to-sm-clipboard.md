---
from: architect
to: sm
date: 2026-09-04
iter: iter-005
---

# Hand-off: Architect → Scrum Master

## Context

[REQ-12](../../../.docs/modules/epaper/prd.md#clipboard) rebound. Product:
[SRS-EP-73](../../../.docs/modules/epaper/features/clipboard/srs-product.md). Logic/UI/quality:
[SRS-EP-31](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) /
[SRS-EP-32](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-32-clipboard-ui) /
[SRS-EP-33](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-33-clipboard-quality).
Hold-still: [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-hold-still).
Decision: [ADR-0037](../../../.docs/adr/ADR-0037-device-clipboard-singleton.md) (supersedes ADR-0024).
BDD: [clipboard.feature](../../../.docs/modules/epaper/features/device-document/bdd/clipboard.feature).

## Review verdict

**READY**

| Class | Finding |
|---|---|
| Strength | Slot vs document vs OS; Actions own orchestration; paste after cut via hold; tap-nudge fixed by the same classifier |
| Concern | 0 wire this track — Infini diverges until a later track (accepted by PM) |
| Gap | none blocking implement |

## Asks

1. [STORY-EP-043](../stories/STORY-EP-043.md) **cancelled**.
2. [STORY-EP-044](../stories/STORY-EP-044.md) **ready** — hold-still + clipboard singleton + actions. No design depends_on.
3. `/dev` EP-044. `/qa` after.

## Constraints

- Do not put copy/cut/paste on `DocContext`. Retire `DeviceDocument::copyToClipboard`.
- 0 `flushWire` for cut/paste. 0 Infini `duplicate_subtree` this story.
- SelectionMode only. No ToolChip.

## Out of scope

- Infini apply, Device Settings, EP-070…072, TRACK-006, OS pasteboard.
