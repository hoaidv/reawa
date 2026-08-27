---
from: qa
to: sm
date: 2026-08-27
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Quality Assurance Engineer → Scrum Master — STORY-EP-059 verified

[STORY-EP-059](../stories/STORY-EP-059.md) (Device inverse undo ring and lastOpId) is **done**. No defect. Did not edit `infini/`. Did not start [STORY-EP-060](../stories/STORY-EP-060.md) (F20/F21 skip catalogue). Chip chrome stays [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md).

## Result

**PASS.** All 12 Gherkin scenarios in [undo-ring.feature](../../../.docs/modules/epaper/features/device-document/bdd/undo-ring.feature) map to `epaper/tests/device_document_test.cpp`. Host suite `epaper/tests/run_device_document_test.sh` exit 0, including `device_document_test: checks passed` (EP-059, 12 scenarios) plus enclose / membership / surround / connector / connector_warp.

Undo host analog: `undo request→restored n=20 p50=500ns p95=1042ns` (bar 500ms).

## Scenario mapping

| Feature scenario | Host test |
|---|---|
| Structural gesture commit pushes one inverse entry | `test_undo_structural_gesture_pushes_inverse_entry` |
| One completed gesture is one undo entry | `test_undo_one_gesture_one_entry` |
| Undo with matching lastOpId restores stored pre-op fields | `test_undo_matching_lastopid_restores_pre_op_fields` |
| Viewport tool selection clipboard-slot and copy do not push undo | `test_undo_viewport_tool_selection_copy_do_not_push` |
| Ring overflow drops the oldest entry | `test_undo_ring_overflow_drops_oldest` |
| Undo requested mid-gesture is deferred | `test_undo_mid_gesture_is_deferred` |
| An accepted document load empties undo and redo | `test_undo_accepted_doc_load_clears_ring` |
| Bound-node drag does not bump connector lastOpId | `test_bound_node_drag_does_not_bump_connector_lastopid` |
| Last-live-pose cache does not bump connector lastOpId | `test_last_live_pose_does_not_bump_connector_lastopid` |
| Ungroup inverse does not delete children | `test_ungroup_inverse_does_not_delete_children` |
| Redo restores the undone forward fields | `test_redo_restores_forward_fields` |
| Empty redo is a no-op | `test_redo_empty_is_noop` |

Extra host probe (not an extra scenario): `test_undo_publishes_counterpart_not_snapshot` (0 `restore_snapshot`; p95 undo analog).

Not a UI story (`ui_spec` empty). No Spec/HTML grey-box spot-check.

## Defects

None.

## Residual risks (not DEF)

- Device/Qt `epaper_bin` not built (`cmake`/`qmake` not on PATH). No RM2 panel.
- Mid-gesture fixture starts at undo depth 1 (create group) rather than Gherkin depth 0; latched undo still applies after commit and restores pre-op ±1 wu.
- `doc_load` fixture is 2 undo + 1 redo, not Gherkin 3 + 2; both stacks empty after accept, and undo cannot reach the pre-load tree.
- Bound-node drag does not assert warp polyline `V` numerically; host calls `refreshAllConnectorWarps` then checks connector `lastOpId` unchanged and `C` not in targets.
- Last-live-pose test writes the cache while endpoints are still present; Gherkin Given says the bound endpoint is absent. Then clauses (lastOpId, ring depth) still hold.
- F20/F21 skip-whole / absence-partial catalogue waits for EP-060.
- Infini emit/applier is [STORY-EP-061](../stories/STORY-EP-061.md) ∥ [STORY-IN-038](../stories/STORY-IN-038.md) — not this sign-off.
- `adlc audit` does not scan `.cpp`/`.hpp` (`CODE_EXTS`); SRS-EP-07/09 may still list as orphan SRS despite `@implements`.

## Asks

1. `/sm` — record EP-059 verified. Do **not** dispatch W3, TRACK-006, or EP-060 from this lane.
2. `/pm` may gate-close this story.

## Constraints

- Did not edit `epaper/` / `infini/` source, PRD, SRS, MASTER, execution board, or tracks.
- Did not git commit.
- Vertical, stop `verified`. Forbidden: REQ-15, REQ-08, CHL-0011, CHL-0012, EP-032, AI, last-writer ADR-0023, TRACK-006 reopen, DeviceMap invert UI, Mouse DragHandler.

## Out of scope

EP-060 F20/F21. EP-061 / IN-038 Infini wire. Chip tiles. W3 erase.
