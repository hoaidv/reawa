---
id: STORY-EP-009
title: Tablet move/resize ghost with ink preview
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — device-side behaviour re-specified under epaper REQ-04…REQ-07; code discarded by the restore. Not scheduled; SM re-slices."
priority: P1
iter: iter-003
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given selection tool move gesture, when dragging, then dashed ghost includes translated ink paths plus bounds rect"
  - "Given resize gesture, when dragging a handle, then ghost shows scaled ink about pickable center with updating bounds"
---

# STORY-EP-009 — Tablet move/resize ghost with ink preview

[SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md) · CHL-0004

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner persona | `dev` |

## Done when (implement)

- `paintInkGhost` draws vector paths under move/resize chrome
- Human verify: content follows the dashed rect while dragging
