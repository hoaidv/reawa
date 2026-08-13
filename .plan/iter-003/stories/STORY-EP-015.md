---
id: STORY-EP-015
title: "Device undo ring"
kind: implement
parent_srs: [SRS-EP-07]
parent_req: [REQ-04]
status: ready
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-EP-014]
acceptance_criteria:
  - "Given a structural op applied through the undo-aware path, When it commits, Then the ring gained exactly one pre-op snapshot and one undo restores that tree exactly (geometry ±1 world unit)."
  - "Given a move that rendered many intermediate frames, When it is released, Then the ring gained exactly one entry and one undo reverts exactly that gesture."
  - "Given 20 structural ops, When a 21st pushes, Then depth stays 20 and the oldest entry is gone."
  - "Given only pan, tool, or selection-state changes, When observed, Then the ring depth is unchanged."
  - "Given undo requested mid-gesture, When the gesture is in progress, Then the gesture is not corrupted and undo applies after commit."
  - "Given a live session, When undo runs, Then a restore_snapshot change is queued and p95 request → restored panel is ≤500 ms."
  - "Given an accepted doc_load, When it completes, Then the ring is empty and undo cannot reach the pre-load tree."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-015 — Device undo ring

Implements the undo half of [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md)
(re-homed from deprecated SRS-IN-12). BDD:
[undo-ring.feature](../../../.docs/modules/epaper/features/device-document/bdd/undo-ring.feature).

Undo *affordance* is **deferred** ([CHL-0010](../challenges/CHL-0010-undo-vs-selection-create-chrome.md)):
this story is the ring itself, with **no on-panel chrome**.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-014 |

## Done when

- All `@SRS-EP-07` undo-ring scenarios green
- No on-panel undo chrome ([CHL-0010](../challenges/CHL-0010-undo-vs-selection-create-chrome.md))
- No EP-020 handshake / Infini edits; `restore_snapshot` is queued locally

**SM 2026-08-13:** EP-014 **done** (RM2 ingest p95=231 µs). This story is **ready**. BDD already lives in `undo-ring.feature` — QA reviews, does not rewrite.
