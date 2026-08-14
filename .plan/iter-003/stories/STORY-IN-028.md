---
id: STORY-IN-028
title: "Desktop handshake-gated doc_load"
kind: implement
parent_srs: [SRS-IN-07, SRS-IN-08]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-IN-027]
acceptance_criteria:
  - "Given Epaper connects and reports queued > 0, When Infini handshakes, Then it sends drain_ack first, applies inbound doc_change in seq order, and sends doc_load only after queue_empty."
  - "Given a completed load, When the rest of the session is traced, Then 0 further outbound document messages are observed (viewport-only downward)."
  - "Given reconnect, When the session returns, Then Infini does not push a document reflexively — it runs the handshake (drain then load)."
  - "Given orientation change or an Infini-side action, When observed, Then 0 doc_load / doc_snapshot messages are sent."
  - "Given the retired names doc_snapshot, pickables, tool_intent, When Infini would have emitted them, Then it does not."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-028 — Desktop handshake-gated doc_load

Implements the load/handshake half of [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md)
per [ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) §4. Retires the
`doc_snapshot` reflex (after edits, orientation, reconnect).

Pairs with device [STORY-EP-020](./STORY-EP-020.md). Viewport publish is already shipped; do not
regress it.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | IN-027 |

## Done when

- Handshake / no-document-after-load scenarios green
- 0 `doc_snapshot` on the wire from Infini
