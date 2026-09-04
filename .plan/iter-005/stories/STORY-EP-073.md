---
id: STORY-EP-073
title: Split clipboard clipops into document helpers and actions
kind: implement
parent_srs: [SRS-EP-31, SRS-EP-07]
parent_req: [REQ-12]
status: draft
priority: P2
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-044]
acceptance_criteria:
  - "Given epaper/drawing/tools/clipboard.hpp after the split, When inspected, Then it holds only the process-global ClipboardSlot and clipboard() accessor (seq, source ids, nodes); no clone/parent/translate/paste orchestration."
  - "Given document query and mutation helpers that clipboard used (slot roots, filter-selected grain, translate, remint, parent walk, SmartGroup local ink, insertAt restore), When relocated, Then they live under epaper/document (DeviceDocument or sibling headers) with names that do not say copy, cut, or paste."
  - "Given CopyAction, CutAction, and PasteAction, When inspected, Then they own copy-to-slot, cut commit, and paste commit (including refuse and dirty); they call document helpers; they do not grow copySelection, cutSelection, or pasteClipboard on DocContext."
  - "Given the host clipboard test suite, When run after the split, Then behavior matches [STORY-EP-044](./STORY-EP-044.md) (same grain, parent, undo, 0 wire)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-073 — Split clipboard clipops into document helpers and actions

**Later.** Do not start until the human picks it. [STORY-EP-044](./STORY-EP-044.md)
(In-document copy cut paste and tap-origin paste) is **done** (human-verified 2026-09-04).
This story is structure only — no product change.

Today `clipops` in [`clipboard.hpp`](../../../epaper/drawing/tools/clipboard.hpp) mixes:

1. Document queries and mutations that copy / cut / paste happen to need.
2. Copy / cut / paste orchestration.
3. The process-global slot.

Target split (human 2026-09-04):

| Goes to | What |
|---|---|
| `epaper/document` (`DeviceDocument` or sibling headers) | Advanced **document** query and mutation. No copy / cut / paste names or slot knowledge. |
| [`copy_action.hpp`](../../../epaper/drawing/tools/actions/copy_action.hpp) / [`cut_action.hpp`](../../../epaper/drawing/tools/actions/cut_action.hpp) / [`paste_action.hpp`](../../../epaper/drawing/tools/actions/paste_action.hpp) | Copy / cut / paste logic (slot fill, cut gesture, paste parent + refuse + dirty). |
| [`clipboard.hpp`](../../../epaper/drawing/tools/clipboard.hpp) | Slot only. |

[ADR-0037](../../../.docs/adr/ADR-0037-device-clipboard-singleton.md) still forbids
`copySelection` / `pasteClipboard` on `DocContext`. Document helpers are tree/query APIs, not
clipboard verbs on the capability port.

Parent [SRS-EP-31](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard)
and [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document).
Requirement [REQ-12](../../../.docs/modules/epaper/prd.md#clipboard).

No design package. No new chrome. Host tests in `epaper/tests/clipboard_test.cpp` stay the contract.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-044](./STORY-EP-044.md) |

## Done when

- Slot-only `clipboard.hpp`
- Generic document helpers under `epaper/document`
- Actions own copy / cut / paste
- Clipboard host tests green; 0 `DocContext` clipboard verbs
