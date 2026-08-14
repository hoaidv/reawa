---
id: STORY-EP-024
title: Undo and Redo buttons on primary toolbar
kind: implement
parent_srs: [SRS-EP-05]
parent_req: [REQ-04]
status: in-review
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-EP-015]
acceptance_criteria:
  - "Given the primary strip, When the panel shows ToolChip, Then layout is sel_rect | sel_freeform | pen | ink_box | 32du gap | undo | redo (64du tiles); undo/redo do not arm a tool."
  - "Given at least one structural commit, When the creator taps Undo, Then the pre-op tree is restored and Redo becomes available."
  - "Given an undone op and no newer commit, When the creator taps Redo, Then that op's tree is restored."
  - "Given a new structural commit after undo, When observed, Then the redo stack is empty and Redo is a no-op."
  - "Given empty undo or redo, When tapped, Then the tree is unchanged (no-op)."
  - "Given undo or redo mid-gesture, When the gesture is in flight, Then the gesture is not corrupted and the last latched history action runs after commit."
design_package: ".plan/iter-003/design/epaper-tool-strip/"
ui_spec: ".plan/iter-003/design/epaper-tool-strip/ui-spec.md"
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-024 — Undo and Redo buttons on primary toolbar

On-panel history actions for [SRS-EP-05](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md)
and redo for [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md).
Decision: [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) / [CHL-0016](../challenges/CHL-0016-undo-redo-toolbar.md).

Human layout: **Selection Rect | Selection Freeform | Pen | Ink-box | ⟨space⟩ | Undo | Redo**.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-015 (ring) |

## Done when

- Layout and hit-test match ADR-0018
- Redo stack host tests green
- No Enclose-as-tool; four exclusive tools unchanged
