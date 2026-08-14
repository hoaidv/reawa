---
feature: device-document
parent_req: [REQ-04, REQ-07]
version: 0.2.0
lifecycle: active
---

# SRS — On-device working document (Logic)

Architect-owned algorithms for [REQ-04](../../prd.md#device-document) (tree, ingestion, undo)
and [REQ-07](../../prd.md#one-way-sync) (inbound classification, load handshake, publish
queue, preview). Ownership: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md).
Wire contract: [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md).
Node kinds and op meanings: [domain/vector-document](../../../../domain/vector-document.md)
— this file does not restate them.
Device-local shapes: [SRS-EP-09](./srs-data.md). Budgets: [SRS-EP-13](./srs-quality.md).
Desktop peer: [SRS-IN-07](../../../infini/features/tablet-sync/srs-logic.md).
Siblings that **write** this tree: [SRS-EP-10 / SRS-EP-11](../ink-box/srs-logic.md).
Viewport map / paint: [SRS-EP-02](../region-sync/srs-logic.md).
Product rules: [srs-product](./srs-product.md) BR-D01…BR-D12.

Wire grammar is canonical in [infini SRS-IN-09](../../../infini/features/vector-document/srs-data.md).
**Do not fork it.** Op type names below are the SRS-IN-09 transmit set.

---

## [SRS-EP-07] Device document, ingestion, op set, and undo ring {#srs-ep-07-device-document}

Parent REQ: [REQ-04](../../prd.md#device-document).

### Endpoint(s)

N/A — device-local, in-memory. No file form on device ([SRS-EP-09](./srs-data.md)).
No message is sent or awaited to ingest, apply, or undo
([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §1). Publication is a
consequence of a committed gesture ([SRS-EP-08](#srs-ep-08-one-way-sync)), never a
precondition.

### In-memory tree

The device holds one `DeviceDocument`: a tree whose kinds, invariants, and op meanings are
those in [domain/vector-document](../../../../domain/vector-document.md). Coordinates in the
tree are **world**. Device-authored kinds this iter: `Ink`, `SmartGroup`. Other kinds arriving
via an accepted `doc_load` are carried opaquely ([SRS-EP-09](./srs-data.md)) and must round-trip
and paint; they are not authored here.

| Rule | Value |
|---|---|
| Lifetime | Session memory only. App restart discards the tree; the next accepted load restores what the desktop had published |
| Writers | This device, via the undo-aware apply path below. The only peer mutation is an accepted `doc_load` ([SRS-EP-08](#srs-ep-08-one-way-sync)) |
| Paint | The panel rasterizes **this** tree ([SRS-EP-02](../region-sync/srs-logic.md)). **0** inbound pictures are a paint source ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2) |
| Geometry agreement | Shared fixtures in [SRS-EP-09](./srs-data.md); divergence is a `CHL-*`, not a local tweak |

### Stroke ingestion (finished stroke → Ink node)

Runs at **pen-up** of a stroke that produced ink (`pen` or `ink_box` armed). Selection
gestures produce no ink and do not ingest. Live samples continue to paint via
[SRS-EP-01](../local-pen-ink/srs-logic.md); ingestion must not sit between a sample and its
pixel (contention bar: [SRS-EP-13](./srs-quality.md)).

```text
1. Take the finished stroke's samples in panel space (post Round 19 map, [SRS-EP-02]).
2. Map each sample to world via the current drawingRegion / orientation.
3. Build an Ink node:
     - polyline in world coordinates
     - every digitizer channel the hardware reported, stored on the node
       (pressure, tilt, distance, timestamp, proximity/button flags, plus extras)
       per [SRS-EP-09](./srs-data.md) Ink sample retention
     - style.strokeWidth in world units ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md))
4. Hand the node to the gesture-commit path (below) as `append_ink`, unless a sibling
   consumes the same pen-up as a different single op (enclose → `create_smart_group`).
5. After the Ink node exists, [SRS-EP-10](../ink-box/srs-logic.md) may decide membership
   (draw-into) or enclose. Those siblings write this tree; they do not own it.
```

| Rule | Value |
|---|---|
| Preview vs node | `stroke_*` on the wire is advisory ([SRS-EP-08](#srs-ep-08-one-way-sync)). The node is the authority and is created only at pen-up |
| Bézier / OCR | Never. Samples stay polylines |
| Ingestion failure | Ink stays painted locally; the tree is unchanged; **0** half-inserted nodes |
| Peer | **0** round trips inside the stroke. No inbound document message is applied between pen-down and commit |

### Op set the device may apply

Closed set, names from [SRS-IN-09](../../../infini/features/vector-document/srs-data.md).
Meanings from [domain/vector-document §Operations](../../../../domain/vector-document.md#operations).
The device applies these locally; it never applies an inbound op except by wholesale
`doc_load`.

| Op | Produced by |
|---|---|
| `append_ink` | Pen-up ingest (this section). Draw-into may set `parentId` on the same op so membership is one gesture |
| `create_smart_group` | [SRS-EP-10](../ink-box/srs-logic.md) enclose / selection-create |
| `create_connector` | [SRS-EP-17](../connector-ink/srs-logic.md) recognition (device **is** an author this campaign) |
| `set_smart_transform` | [SRS-EP-11](../ink-box/srs-logic.md) move / resize commit |
| `set_ink_scale_mode` | [SRS-EP-11](../ink-box/srs-logic.md) toggle |
| `reparent` | [SRS-EP-10](../ink-box/srs-logic.md) when membership cannot be expressed as `append_ink.parentId` |
| `remove_node` | Empty-group cleanup ([SRS-EP-10](../ink-box/srs-logic.md)); a `SmartGroup` left with zero children is removed in the **same** gesture as the child removal |
| `restore_snapshot` | Undo (this section) — how undo publishes ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §5) |

Unknown `type` values are not applied. An op that would violate a domain invariant is
**rejected**: tree unchanged, ring unchanged, queue unchanged.

Desktop-only ops in SRS-IN-09 (`insert_node`, `set_text`) are not in
the device author set this iter. If they arrive inside an accepted `doc_load`, the nodes
they describe are opaque carry-through, not live-authored.

### Gesture-commit path (undo-aware apply)

One completed **gesture** is one commit. Recognition, membership, and manipulation
([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) choose **which**
op a gesture produces; this path is the only way that op enters the tree.

```text
validate op against domain invariants
  fail → reject (tree, ring, queue unchanged); stop
if a structural op will apply:
  if ring depth == 20 → drop the oldest entry
  push { snapshot: current tree, opId, kind }     # pre-op snapshot
  apply op to the tree
  enqueue one doc_change for this op              # [SRS-EP-08]
```

| Rule | Value |
|---|---|
| Granularity | Exactly **1** ring entry per completed gesture. Intermediate manipulation frames are local paint only — they do not push |
| Snapshot | Whole-document snapshot ([SRS-EP-09](./srs-data.md) `{ snapshot, opId, kind }`). Inverse-op algebra is not used ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §5) |
| Depth | **20**. Overflow drops the oldest. Undo past an empty ring is a **no-op**, not an error |
| Not covered | Viewport pan/zoom, tool switches, selection changes — these are not document state and **must not** push |
| Redo | Snapshot stack, depth **20**. Undo pushes the current tree onto redo before restore. A successful structural commit **clears** redo. Empty redo is a no-op. Branching / non-linear history remains out ([ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md)) |

### Undo

```text
if a gesture is in flight:
  latch the request; do not mutate the in-flight gesture
  after that gesture commits (one ring entry, one queued change), run undo
if ring is empty: no-op
else:
  push the current tree onto the redo stack (drop oldest if depth == 20)
  pop the newest undo entry
  replace the tree with that entry's snapshot
  enqueue doc_change { op: restore_snapshot { document: restored tree } }
```

### Redo

```text
if a gesture is in flight:
  latch; last history request wins; run after commit
if redo stack is empty: no-op
else:
  push the current tree onto the undo ring (drop oldest if depth == 20)
  pop the newest redo entry
  replace the tree with that entry's snapshot
  enqueue doc_change { op: restore_snapshot { document: restored tree } }
```

| Rule | Value |
|---|---|
| Exactness | After undo, the tree equals the popped pre-op snapshot (0 divergent nodes). Geometry tolerance is [SRS-EP-13](./srs-quality.md), not redefined here |
| Mid-gesture | Deferred as above. **0** corrupted in-flight gestures |
| Already published | Undo is itself a change and publishes like any other — the mirror follows |
| Accepted `doc_load` | Undo and redo stacks are emptied. History cannot reach the pre-load tree |

### No peer round trip inside a gesture

From pen-down until the gesture-commit path returns: **0** inbound document-bearing
messages are applied, and **0** `doc_change` are emitted. Preview `stroke_*` may flow up.
A `doc_load` offered during the gesture is deferred by [SRS-EP-08](#srs-ep-08-one-way-sync).

### Errors / partial failure

| Case | Behavior |
|---|---|
| Ingestion failure | Ink stays painted; document unchanged; 0 half-inserted nodes |
| Rejected op (invariant) | 0 published, 0 ring entries pushed |
| Undo on empty ring | No-op |
| Link down during ingest / undo | Completes locally; the change joins the publish queue |
| App restart | Unpublished tree is gone — accepted ([srs-product](./srs-product.md) BR-D07) |

---

## [SRS-EP-08] One-way sync: inbound classification, load handshake, publish queue, preview {#srs-ep-08-one-way-sync}

Parent REQ: [REQ-07](../../prd.md#one-way-sync).

Desktop counterpart: [SRS-IN-07](../../../infini/features/tablet-sync/srs-logic.md).
Contract: [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md). Envelopes:
[SRS-IN-09](../../../infini/features/vector-document/srs-data.md). Latency and
inbound-zero bars: [SRS-EP-13](./srs-quality.md).

### Endpoint(s)

JSON-lines TCP session to Infini (same transport as [SRS-EP-02](../region-sync/srs-logic.md)).
Auth: none in v0 (trusted local link).

### Inbound classification

Every inbound message is classified by `type` before any document mutation.

| `type` | Action |
|---|---|
| `viewport` | Apply always — map only, [SRS-EP-02](../region-sync/srs-logic.md). Never document content |
| `drain_ack` | Handshake only — start publishing the queue |
| `doc_load` | Accept **only** when the handshake below says the load is legal. Otherwise reject |
| `doc_snapshot` | **Reject** — retired. Log once per session, surface in the status line ([SRS-EP-05](../tool-modes/srs-ui.md)) |
| `pickables` (field or message) | **Reject** — retired with [SRS-IN-13](../../../infini/features/tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport) |
| `tool_intent` | **Reject** — retired with SRS-IN-13 |
| `region_refresh` | Ignore (legacy PNG); log once; paint stays on the local tree |
| `debug_request` / `debug_start` / `debug_stop` / `debug_log` | **Reject on this socket** — debug family is TCP `:9878` ([SRS-EP-15](#srs-ep-15-debug-log-ship)). Same as unknown type. **Not** in [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) |
| anything else | Reject, log as a protocol defect, surface in the status line |

After a session has **accepted** its initial `doc_load`, inbound document-bearing messages
for the rest of that epoch must be **0**. Viewport continues. An unsolicited `doc_load`
mid-session (no handshake in progress) is a protocol defect: local document unchanged,
logged, surfaced — not applied.

`stroke_begin.intent` is not an inbound type; if present on an outbound preview it is a
device defect — do not send it.

### Load handshake

A full load **replaces** the local document, so it is legal only at an epoch boundary and
only when unpublished work cannot be clobbered
([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) §4).

```text
Epaper                                   Infini
  |-- hello { lastSeq, queued: k } ------->|
  |<-- drain_ack -------------------------|   (when k > 0)
  |-- doc_change × k (in seq order) ------>|
  |-- queue_empty ------------------------>|
  |<-- doc_load { document, seq: 0 } -----|   (only now)
  |-- load_ack --------------------------->|
```

| Rule | Value |
|---|---|
| `k = 0` | Load is legal after `hello` (the device already has nothing to drain). `queue_empty` may be omitted |
| `k > 0` | Device publishes all k changes in `seq` order, then `queue_empty`, **then** accepts `doc_load`. **0** queued changes discarded |
| Unsolicited mid-session | No handshake in flight → reject (classification table) |
| Gesture in flight | Defer the load until the gesture commits. If that commit enqueues a change, that change publishes (and `queue_empty` is sent) **before** the load is accepted |
| Malformed `doc_load` | Current document intact; no `load_ack`; state visible |

Do not merge queued changes into the load. Draining first is the v1 safety rule; merging
is multi-directional sync and is out of scope.

### Accepted load = new epoch

On a legal, well-formed `doc_load`:

| Effect | Value |
|---|---|
| Tree | Replaced wholesale by `document` |
| `seq` | Reset to **0**. Next committed op is `seq: 1`, `baseSeq: 0` |
| Undo ring | Cleared |
| Selection | Cleared |
| Publish queue | Empty (it drained before accept) |
| Opaque kinds | Preserved verbatim for later republish ([SRS-EP-09](./srs-data.md)) |
| Ack | Send `load_ack`. Publishing of new ops may resume |

### Publish queue

Ordered, in-memory, retained across a link drop (not across app restart).

Envelope (do not fork):

```text
{ "type": "doc_change", "seq": n, "opId": "<uuid>", "op": { … SRS-IN-09 envelope … }, "baseSeq": n-1 }
```

| Rule | Value |
|---|---|
| When | Exactly one `doc_change` per committed gesture-op from [SRS-EP-07](#srs-ep-07-device-document) |
| Order | Strict `seq`. Never coalesce, never reorder |
| Idempotency | `opId` is stable. Retransmitting a queued entry reuses the same `seq` + `opId`. A duplicate apply on the mirror is a no-op ([SRS-IN-07](../../../infini/features/tablet-sync/srs-logic.md) / [SRS-IN-04](../../../infini/features/vector-document/srs-logic.md)) |
| Link down | Editing continues; every commit enqueues; **0** tools gated on the session ([srs-product](./srs-product.md) BR-D04) |
| Reconnect | `hello` → `drain_ack` → publish the queue in `seq` order → `queue_empty`. **0** lost, **0** reordered |
| Pending visibility | Non-empty queue is a visible pending state ([SRS-EP-05](../tool-modes/srs-ui.md)). Queued changes are never silently dropped ([srs-product](./srs-product.md) BR-D11) |
| Undo | Publishes as `restore_snapshot` like any other committed op |

### Preview strokes are not document changes

While a stroke is in progress the device may stream `stroke_begin` / `stroke_point` /
`stroke_end` so the desktop stays live ([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) §6).

| Rule | Value |
|---|---|
| `stroke_begin.intent` | **Not sent** |
| Document | Preview paths are **never** inserted into the device tree, never queued as `doc_change`, never an undo entry |
| Authority | The Ink node (or the enclose/`create_smart_group` outcome) reaches the desktop only in the `doc_change` at pen-up |
| Direction | Preview never flows back to the device |

### Errors / partial failure

| Case | Behavior |
|---|---|
| Retired / unknown inbound type | Not applied; logged once per session; status line |
| Unsolicited `doc_load` | Local document unchanged |
| Load while queue non-empty | Drain first; 0 discards |
| Load mid-gesture | Gesture commits; its change publishes; then handshake may accept |
| Session drop mid-gesture | Gesture completes and commits locally; change queued |
| App restart | Queue lost with the tree; next load restores previously published work |

---

## [SRS-EP-15] Debug-log ship (TCP :9878) {#srs-ep-15-debug-log-ship}

Parent REQ: [REQ-07](../../prd.md#one-way-sync) (session/debug of the live link — not a
new REQ). Desktop peer: [SRS-IN-17](../../../infini/features/tablet-sync/srs-logic.md#srs-in-17-debug-log-channel).
Quality: [SRS-EP-16](./srs-quality.md#srs-ep-16-debug-log-ship-quality).

Ship Qt console (and stdout/stderr when captured) to Infini **when requested**, on a
**sidecar** JSON-lines TCP `:9878`. **Not** a document channel. **Not** `doc_change`.
**Not** Smart Group. Does **not** amend
[ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md). Does **not** change
[SRS-EP-10](../ink-box/srs-logic.md) recognition.

### Endpoint(s)

| Role | Bind / connect | Gate |
|---|---|---|
| Infini | listen `:9878` | [SRS-IN-17](../../../infini/features/tablet-sync/srs-logic.md#srs-in-17-debug-log-channel) |
| Epaper | connect `{RM_SYNC_HOST}:{port}` | Env `EPAPER_DEBUG_LOG` in `{1,true,on,yes}` (case-insensitive). **Default off** |

Port: `EPAPER_DEBUG_PORT` if set, else `INFINI_DEBUG_PORT` if present in the process
environment, else **9878**. Document session stays on `:9877` ([SRS-EP-08](#srs-ep-08-one-way-sync)).

Env off: **0** connects to `:9878`, **0** debug sockets, Qt default handler may still
print locally. Env on: connect when `RM_SYNC_HOST` is known; wait idle until `debug_start`.

### Closed ids — :9878 only

Same four types as [SRS-IN-17](../../../infini/features/tablet-sync/srs-logic.md#srs-in-17-debug-log-channel):
`debug_request`, `debug_start`, `debug_stop` (inbound), `debug_log` (outbound). Envelope
canonical there — do not fork.

| Rule | Value |
|---|---|
| Shipping | Off until `debug_start`; off after `debug_stop`; both idempotent |
| `:9877` | **0** `debug_*` sent or accepted as traffic. If a `debug_*` `type` arrives on the document socket, reject as protocol defect (classification table above) |
| Document | **0** `debug_log` lines applied to `DeviceDocument`. **0** enqueue as `doc_change` |

### Worker thread / queued connection

All `:9878` reads and writes run on a **worker thread** (or Qt queued connection to a
socket-owning object that is **not** the GUI/render thread).

```text
qInfo / qWarning / qCritical  ─┐
stdout / stderr capture       ─┼─→ wait-free / try-lock enqueue → worker → TCP :9878
[enclose] qInfo (below)       ─┘
```

| Rule | Value |
|---|---|
| Message handler | Custom `qInstallMessageHandler` when env on. The handler **returns without blocking on I/O**. Enqueue or drop |
| GUI / render / ink thread | **0** `write()` / `flush()` / waiting socket syscalls |
| `ingestPoint` / paint | **0** log I/O ([SRS-EP-16](./srs-quality.md#srs-ep-16-debug-log-ship-quality)) |
| Queue | Bounded **512** records. Overflow drops **oldest**, increments the next `debug_log.dropped` |
| Qt queued connection | Allowed for "handler → worker"; **not** allowed as a blocking wait from paint |

### Sources shipped

| Source | `logger` | `level` | Required |
|---|---|---|---|
| `qInfo` | `qt` | `info` | **yes** when shipping |
| `qWarning` | `qt` | `warning` | **yes** |
| `qCritical` / `qFatal` | `qt` | `critical` | **yes** (`qFatal` still aborts after enqueue attempt) |
| stdout | `stdio` | `stdout` | **yes, if fd redirect succeeds** |
| stderr | `stdio` | `stderr` | **yes, if fd redirect succeeds** |

Stdout/stderr: attempt to capture process fds when env on (pipe + reader on the worker,
or equivalent). If redirect **fails**, continue shipping Qt messages and emit **one**
`debug_log` `{ logger: "qt", msg: "[debug] stdio capture unavailable" }`. Do not crash.
Do not install a handler that writes the debug socket from the ink thread as a fallback.

### `[ink]` and `[enclose]` log sources (not recognizer changes)

Logging is a **side channel** after pen-up dispatch. It must not alter
`recognize_enclose` guards, `create_smart_group` payloads, membership, or published ops.

| When (latched tool at pen-down) | Log | Path |
|---|---|---|
| `pen` (ordinary ink) | Exactly one `qInfo` `[ink] id=<inkId>` after successful `append_ink` | [SRS-EP-07](#srs-ep-07-device-document) ingest only — **do not** call enclose recognition |
| `ink_box` | Exactly one `qInfo` whose `msg` starts with `[enclose]` after enclose ingest returns | [SRS-EP-10](../ink-box/srs-logic.md) |
| `selection` | No stroke ingest → **0** `[ink]` / `[enclose]` lines | — |

`[enclose]` tokens:

| Token | Meaning |
|---|---|
| `armed` | Always `ink_box` for this line |
| `outcome` | `created` \| `stayed_ink` |
| `guard` | `none` \| `size` \| `content` \| `already_grouped` — when stayed_ink |
| `id` | Smart Group id when `outcome=created` |
| `children` | `[id,…]` boundary then content ink ids when `outcome=created` |
| `captured` | Content ink count (children minus boundary) when created; `0` when stayed_ink |

Example (informative):

```text
[ink] id=stroke_42
[enclose] armed=ink_box outcome=created id=sg_enclose_stroke_99 children=[stroke_99,stroke_42,stroke_7]
[enclose] armed=ink_box outcome=stayed_ink guard=size captured=0
```

| Rule | Value |
|---|---|
| Pen / non-ink_box | **0** calls into enclose recognition for logging; **0** `[enclose]` lines |
| Path | `qInfo` → debug handler/queue. **Not** a `:9877` message |
| Recognizer | Logging must not change [SRS-EP-10](../ink-box/srs-logic.md) verdicts |

### Errors / partial failure

| Case | Behavior |
|---|---|
| Env off | No debug client. Local Qt logging unchanged |
| Infini not listening | Connect fails; retry with backoff on the worker; **0** effect on ink / document / `:9877` |
| `debug_stop` | Stop emitting; keep the socket if up |
| Backpressure | Drop oldest; never block paint; next emitted line carries `dropped` |
| Unknown inbound `:9878` type | Drop |
| ADR-0015 type on `:9878` | Drop; **0** apply to `DeviceDocument` |

---

## Superseded

New sections. They inherit, on the device:

- infini [SRS-IN-12](../../../infini/features/vector-document/srs-logic.md#srs-in-12-undo-history)
  (deprecated) — snapshot ring, depth 20, "not covered" list. The shared-timeline / remote-op
  row is dropped: one writer.
- the device half of [SRS-IN-13](../../../infini/features/tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport)
  (retired) and the document-authority half of [SRS-IN-07](../../../infini/features/tablet-sync/srs-logic.md)
  as it stood before 2026-08-13.

See the [lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).
Established 2026-08-13 by [CHL-0009](../../../../../.plan/iter-003/challenges/CHL-0009-missing-device-document-srs-logic.md)
(completing the assignment in the 2026-08-13 architect handoff).
