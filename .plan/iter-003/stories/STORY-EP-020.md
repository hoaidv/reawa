---
id: STORY-EP-020
title: "Device one-way sync handshake and publish queue"
kind: implement
parent_srs: [SRS-EP-08]
parent_req: [REQ-07]
status: draft
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-014]
acceptance_criteria:
  - "Given a session that has completed its initial load, When the lifetime is traced, Then 0 inbound document-bearing messages are observed and viewport messages continue to apply."
  - "Given a live session, When an unsolicited doc_load, doc_snapshot, pickables, or tool_intent arrives, Then it is not applied, it is logged, and the local document is unchanged."
  - "Given 5 queued changes, When the desktop offers a load, Then all 5 publish in seq order, queue_empty is sent, then the load is accepted (0 queued discarded); seq resets; undo ring and selection clear; load_ack is sent."
  - "Given the session down, When the creator performs 10 document operations, Then all 10 apply locally, all 10 queue in order, and 0 tools were unavailable."
  - "Given a restored link, When drain_ack arrives, Then queued ops publish in seq order (0 lost, 0 reordered) and duplicate opId apply is a no-op."
  - "Given a committed structural op, When it publishes, Then exactly one doc_change {seq, opId, op, baseSeq} is emitted and the mirror updates p95 ≤300 ms."
  - "Given a stroke in progress, When stroke_* streams, Then no stroke_begin carries intent, the node arrives only in the pen-up doc_change, and 0 preview paths are written to the mirror."
  - "Given a gesture in progress, When a doc_load is offered, Then the gesture commits first and the change publishes before the load is accepted."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-020 — Device one-way sync handshake and publish queue

Implements [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md)
per [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md).
BDD: [one-way-sync.feature](../../../.docs/modules/epaper/features/device-document/bdd/one-way-sync.feature).

Last on the device (architect ask 3): editing works offline by design. Pairs with
[STORY-IN-027](./STORY-IN-027.md) / [STORY-IN-028](./STORY-IN-028.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-014 |

## Done when

- `@SRS-EP-08` scenarios green
