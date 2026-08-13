---
id: STORY-EP-008
title: Tablet handle resize tool_intent
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — device-side behaviour re-specified under epaper REQ-04…REQ-07; code discarded by the restore. Not scheduled; SM re-slices."
priority: P1
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given selection tool and a selected pickable, when the user drags a corner/edge handle, then Epaper sends tool_intent action=resize with world AABB {x,y,width,height}"
  - "Given resize release, when Infini applies the intent, then Smart Group transform updates via smartTransformFromWorldAabb"
---

# STORY-EP-008 — Tablet handle resize tool_intent

[SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md) · CHL-0004

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner persona | `dev` |

## Done when (implement)

- `hitResizeHandle` + resize gesture FSM in TabletCanvasItem
- Resize commit emits `tool_intent` resize bounds
- Human verify: tablet handles resize the box on both sides
