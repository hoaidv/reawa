---
id: STORY-EP-061
title: Device undo queue is counterpart compound not restore snapshot
kind: implement
parent_srs: [SRS-EP-08, SRS-EP-07]
parent_req: [REQ-07]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-059]
acceptance_criteria:
  - "Given undo or redo of a matching entry, When the device records the outbound change, Then it is the counterpart op or a compound of counterparts — 0 restore_snapshot on that path."
  - "Given Path A stroke-erase, When it commits and later undoes, Then the forward/inverse use set_ink_samples (and remove_node only if the ink was emptied) — 0 snapshot fallback."
  - "Given this iter, When undo/redo runs, Then Infini is not required to apply those ops (tablet→desktop undo sync is out; [STORY-IN-038](./STORY-IN-038.md) cancelled)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-061 — Device undo queue is counterpart compound not restore snapshot

TRACK-005. [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-08-one-way-sync). Parent [REQ-07](../../../.docs/modules/epaper/prd.md#one-way-sync).

Human 2026-08-27: **do not** implement tablet→desktop apply this iter. [STORY-IN-038](./STORY-IN-038.md) is **cancelled**. This story is the **device queue**: undo/redo must not record `restore_snapshot`. Infini mirror apply waits for an independent sync algorithm in a later phase.

`restore_snapshot` remains last-resort **non-undo** only.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-059 |

## Done when

- 0 `restore_snapshot` on undo/redo queue
- `compound` / `set_ink_samples` recorded on the device as specified
- `infini/` not edited

Verified 2026-08-27: Quality Assurance Engineer **PASS**. Host map: `.docs/modules/epaper/features/device-document/bdd/undo-queue.feature` (5 scenarios). Infini apply remains cancelled ([STORY-IN-038](./STORY-IN-038.md)). Human verified on device 2026-08-27.
