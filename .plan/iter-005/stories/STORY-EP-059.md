---
id: STORY-EP-059
title: Device inverse undo ring and lastOpId
kind: implement
parent_srs: [SRS-EP-07, SRS-EP-09]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a completed structural gesture, When it commits, Then the ring gained exactly one UndoEntry { forwardOpId, seq, inverses, targets } and 0 whole-tree snapshots."
  - "Given that entry's targets, When lastOpId still equals the forward opId, Then one undo applies stored absolute pre-op fields (geometry ±1 world unit; 0 divergent nodes)."
  - "Given 20 structural ops, When a 21st pushes, Then depth stays 20 and the oldest entry is gone."
  - "Given only pan, tool, selection, or clipboard-slot changes, When observed, Then the ring depth is unchanged (copy = 0 entries)."
  - "Given undo requested mid-gesture, When the gesture is in progress, Then the gesture is not corrupted and undo applies after commit."
  - "Given an accepted doc_load, When it completes, Then both undo and redo stacks are empty."
  - "Given a bound-node drag that re-derives connector warp, When the box pose commits, Then connector lastOpId is unchanged (derived warp / last-live-pose do not count)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-059 — Device inverse undo ring and lastOpId

Replaces shipped [STORY-EP-015](../../iter-003/stories/STORY-EP-015.md) snapshot behaviour. Do **not** extend that ring. [ADR-0032](../../../.docs/adr/ADR-0032-inverse-op-undo.md) accepted. Chip chrome stays [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md) — this story is the ring, not the tiles.

Parent [REQ-04](../../../.docs/modules/epaper/prd.md#device-document), [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document), [SRS-EP-09](../../../.docs/modules/epaper/features/device-document/srs-data.md#srs-ep-09-device-data). Quality Assurance Engineer retags [undo-ring.feature](../../../.docs/modules/epaper/features/device-document/bdd/undo-ring.feature) in this wave.

Honor the counterpart table: ungroup ≠ delete children; connector warp is derived; copy = 0 entries. No `restore_snapshot` on undo. Device queue of counterparts is [STORY-EP-061](./STORY-EP-061.md). Infini apply ([STORY-IN-038](./STORY-IN-038.md)) is **cancelled** this iter.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- All `@SRS-EP-07` / `@SRS-EP-09` inverse-ring scenarios green
- 0 whole-tree snapshots on the device ring
- No on-panel chrome change
- `infini/` not required for this story (host `DeviceDocument` tests)
