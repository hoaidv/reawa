---
id: STORY-EP-044
title: In-document copy cut paste and tap-origin paste
kind: implement
parent_srs: [SRS-EP-31, SRS-EP-32, SRS-EP-33, SRS-EP-11]
parent_req: [REQ-12]
status: done
priority: P0
iter: iter-005
estimate: 8
owner: dev
depends_on: []
acceptance_criteria:
  - "Given SelectionMode and a non-empty selection, When copy then tap a destination and paste, Then new ids exist, union AABB top-left equals the tap (±1 px @ 100%), source unchanged, 0 wire."
  - "Given a tap-selected ink-box, When copy then paste, Then the copy includes the box’s children."
  - "Given free ink in the slot, When tap an ink-box (not a live source) and paste, Then the copy’s parent is that SmartGroup."
  - "Given a non-empty selection, When cut then tap a destination and paste, Then originals are gone after cut; one undo of paste removes copies; second undo restores originals; empty groups left."
  - "Given empty clipboard, When paste is invoked or the creator taps empty, Then 0 nodes change; 0 paste chrome."
  - "Given pointer-down on a node with travel ≤1 mm and lift, When observed, Then the node is selected and its world pose is unchanged."
  - "Given copy then tap a live source, When paste is tapped, Then 0 nodes change, refuse is shown, slot kept."
  - "Given a freeform or marquee selection, When the toolbar is shown, Then paste is absent."
  - "Given a non-empty selection, When the creator taps empty canvas, Then selection clears and 0 paste chrome appears."
---

# STORY-EP-044 — In-document copy cut paste and tap-origin paste

TRACK-005. Parent [REQ-12](../../../.docs/modules/epaper/prd.md#clipboard). Product
[SRS-EP-73](../../../.docs/modules/epaper/features/clipboard/srs-product.md). Logic
[SRS-EP-31](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard).
UI [SRS-EP-32](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-32-clipboard-ui).
Tap vs travel [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-hold-still).
ADR [ADR-0037](../../../.docs/adr/ADR-0037-device-clipboard-singleton.md). BDD
[clipboard.feature](../../../.docs/modules/epaper/features/device-document/bdd/clipboard.feature).
Adopted [CHL-0031](../challenges/CHL-0031-clipboard-tap-paste.md).

No design `depends_on` (EP-043 cancelled). Infini apply out of this story.

**Human verified complete** 2026-09-04 (device). Follow-up structure: [STORY-EP-073](./STORY-EP-073.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |
