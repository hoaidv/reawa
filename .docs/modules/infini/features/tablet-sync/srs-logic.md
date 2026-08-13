---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.7.0
lifecycle: active
---

# SRS — Tablet sync Infini (Logic)

Binds Infini to the session contract in
[ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) under the ownership model of
[ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md), with stroke paint parity from
[ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md).
Shared node semantics: [domain/vector-document](../../../../domain/vector-document.md).
Device peer: [epaper/device-document](../../../epaper/features/device-document/srs-logic.md).

**Code SoT (2026-08-11):** `infini/src/session/TabletSession.ts`,
`infini/src/canvas/CanvasStage.tsx`, `infini/src/canvas/Viewport.ts`,
`infini/electron/main.cjs` (TCP `:9877`). **The 0.6.0 contract below is target, not shipped** —
shipped code still speaks `doc_snapshot` and has no inbound change applier.

## [SRS-IN-07] Session roles and channel binding

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014 / ADR-0015. Downward traffic narrowed to
     handshake-gated doc_load + viewport; new inbound doc_change applier; pickables removed.
     Same id, content revised; no supersession. -->

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

> **Revised 2026-08-13.** Infini is no longer the document authority
> ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md)). It **loads** the document
> to the device once, **drives the viewport**, and **applies** the device's published changes to its
> mirror. The pre-rework rules that pushed `doc_snapshot` after Infini edits, on orientation change,
> and on reconnect are withdrawn — that reflex is what clobbered device work.

### Endpoint(s)

Local network session to RM2 (USB Ethernet). Framing: **JSON-lines** over TCP.
Auth: none in v0 (trusted local link). Default listen: `0.0.0.0:9877`.

### Channels (ADR-0015 contract)

| Channel | Owner (writer) | Consumer | Cardinality / priority |
|---|---|---|---|
| **Viewport** | Infini | Epaper | Continuous, ≤30 Hz — apply before next pen sample |
| **Load** | Infini | Epaper | **Once** per session start / reconnect / explicit resync, handshake-gated |
| **Change** | Epaper | Infini | One `doc_change` per committed op |
| **Preview stroke** | Epaper | Infini | Continuous during a stroke — advisory, never persisted |

**Invariant (testable by message type):** after the load completes, Infini sends **0** document
messages for the rest of the session. Only `viewport` may flow down.

### Tablet drawing frame (CSS) and `drawingRegion` (world)

| Concept | Space | Meaning |
|---|---|---|
| Tablet drawing frame | CSS px | Max-fit centered rect matching panel aspect for current gut pose |
| `drawingRegion` | World AABB | `frameWorldAabb(frame, viewport)` |

Panel aspect: RM2 native `1404×1872`. Tall guts → portrait aspect; wide guts → landscape aspect.

### Orientation (four gut poses)

| Id | Frame | invertX | invertY | Notes |
|---|---|---|---|---|
| `gutToLeft` | tall | false | false | **Default** — verified vertical |
| `gutOnTop` | wide | false | false | Landscape unwrap + L/R u-flip |
| `gutAtBottom` | wide | true | true | |
| `gutToRight` | tall | true | true | |

Legacy aliases on Epaper ingest: `portrait`→`gutToLeft`, `landscape`→`gutOnTop`.

`panelToFrameUv` maps post-digitizer **panel** coords → frame UV for RM→Infini strokes.

### Drawing-region marker

| State | Marker |
|---|---|
| Idle | Not visible |
| Pan/zoom active | Visible outline of CSS frame |
| Gesture end | Hide after **100 ms** settle debounce (no fade required) |

### Viewport message (Infini → Epaper)

| Field | Required | Meaning |
|---|---|---|
| `type` | yes | `viewport` |
| `translate` | yes | `{ x, y }` |
| `scale` | yes | uniform > 0 |
| `drawingRegion` | yes | World AABB of tablet frame |
| `seq` | yes | monotonic |
| `orientation` | yes (shipped) | gut id string |
| `settle` | on flush | `true` when gesture settle / force flush |

### Viewport publish coalesce

| Rule | Value |
|---|---|
| Continuous pan/zoom | ≤ **30 Hz**; latest wins |
| Gesture end / force | `flushViewport` — bypass rate limit; `settle: true` |
| Wheel settle | ~100 ms debounce then flush |

### Document load (Infini → Epaper)

```text
{ "type": "doc_load", "document": { … ADR-0010 tree … }, "seq": 0 }
```

Renamed from `doc_snapshot` **deliberately** ([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md)
§1): a different name for a different guarantee, so no code path can keep the old
"snapshot is truth, at any time" behaviour by accident.

| Rule | Value |
|---|---|
| Legal when | Session start, reconnect, or explicit resync — **and** only after `queue_empty` (or the device reported `queued: 0`) |
| Illegal | Unsolicited mid-session; after an Infini-side action; on orientation change. The device rejects these and logs a protocol defect |
| Effect on device | Wholesale replace; `seq` resets to 0 for the new session epoch |
| Content | The full tree, not a WorldLayer primitive list — both peers hold the [domain model](../../../../domain/vector-document.md) |

### Reconnect handshake

```text
Epaper                                   Infini
  |-- hello { lastSeq, queued: k } ------->|
  |<-- drain_ack -------------------------|
  |-- doc_change × k (in order) ---------->|
  |-- queue_empty ------------------------>|
  |<-- doc_load { document, seq: 0 } -----|
  |-- load_ack --------------------------->|
```

Draining before loading is what makes "replace the local document" safe. Infini must not offer a
load while the device reports queued changes.

### Document change (Epaper → Infini)

```text
{ "type": "doc_change", "seq": n, "opId": "<uuid>", "op": { … }, "baseSeq": n-1 }
```

| Rule | Value |
|---|---|
| Apply | Idempotent by `opId` ([SRS-IN-04](../vector-document/srs-logic.md)); duplicate = no-op |
| Order | Strict `seq`; `baseSeq` ≠ last applied `seq` ⇒ **gap** |
| On gap | Mirror is unsafe → request an explicit resync; do **not** guess or reorder |
| Unknown `op` type | Log, do not crash, mark the mirror **suspect** and surface it. A suspect mirror must not be saved silently |
| Ops carried | `append_ink`, `create_smart_group`, `set_smart_group_transform`, `set_ink_scale_mode`, `reparent`, `remove`, `restore_snapshot` |
| Mirror latency | p95 ≤300 ms from device commit to mirror applied ([SRS-IN-08](./srs-quality.md)) |

### Preview stroke stream (Epaper → Infini)

| Message | Fields |
|---|---|
| `stroke_begin` | `id`, `brush.width` (**world** units), optional `cw`/`ch` panel size |
| `stroke_point` | `id`, `x`, `y` (**panel** after Round 19 map), optional `p` |
| `stroke_end` | `id` |

Infini maps panel→UV→`drawingRegion` world and renders a **transient preview path** keyed by stroke
id, so the desktop keeps its shipped ≤50 ms liveness. The preview is replaced by the real node when
the `doc_change` carrying it arrives.

| Rule | Value |
|---|---|
| Lifetime | `stroke_begin` until the matching `doc_change`, or session drop |
| Orphaned preview | Dropped on the next load or after a bounded timeout — never persisted |
| Mirror / save | Preview paths are **never** written to the mirror or serialized |
| Authority | None. Safe precisely because the previewer is not the writer ([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) §6) |

`stroke_begin.intent` is **removed** — the device interprets its own stroke ([SRS-IN-13] retired).

### Stroke paint (Infini)

Per [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md):

- Ink / path `strokeWidth` is **world units**.
- Canvas: CSS thickness ≈ `strokeWidth_world * viewport.scale`.
- Zoom in → thicker; zoom out → thinner.

### Closed ids (message types) — ADR-0015 v1

| id | Channel | Direction |
|---|---|---|
| `viewport` | viewport | Infini → Epaper |
| `doc_load` | load — handshake-gated | Infini → Epaper |
| `drain_ack` | session | Infini → Epaper |
| `hello` / `queue_empty` / `load_ack` | session | Epaper → Infini |
| `doc_change` | change | Epaper → Infini |
| `stroke_begin` / `stroke_point` / `stroke_end` | preview | Epaper → Infini |

### Retired / rejected ids

| id | Status |
|---|---|
| `doc_snapshot` | **Retired** — replaced by `doc_load` with a narrower legality rule |
| `doc_snapshot.pickables` | **Retired** with [SRS-IN-13](#srs-in-13-tool-intent-transport) |
| `tool_intent` | **Retired** with [SRS-IN-13](#srs-in-13-tool-intent-transport) |
| `stroke_begin.intent` | **Retired** — the device interprets its own stroke |
| `doc_op` / `doc_ack` | Superseded by `doc_change`; a later multi-directional ADR may revive `doc_op` |
| `region_refresh` | Legacy PNG — **do not send**; Epaper ignores |
| `debug_request` / `debug_start` / `debug_stop` / `debug_log` | **Rejected on :9877** — sidecar TCP `:9878` ([SRS-IN-17](#srs-in-17-debug-log-channel)). Arriving here is a protocol defect like any unknown type. **Not** in [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) |

### Errors / partial failure

| Case | Behavior |
|---|---|
| No native bridge | Browser-only; no RM sync |
| Bad JSON line | Log; continue |
| Unknown host→RM type | Epaper ignores and logs a protocol defect |
| RM disconnect | Status update. **Do not** push a document on reconnect — run the handshake: drain the device queue first, then offer `doc_load` |
| `doc_change` gap (`baseSeq` mismatch) | Mirror marked suspect; request explicit resync; do not save a suspect mirror |
| Device reports `queued: k > 0` | Send `drain_ack`, ingest k changes in order, wait for `queue_empty` before any load |

### Other logic

- Live paint: `InfiniDocument` WorldLayer, fed by applied `doc_change` ops plus transient stroke
  previews. Infini authors **no** document changes of its own
  ([REQ-04](../../prd.md#smart-group) deprecated).
- `TabletSession` holds the mirror `VectorDocument`; persistence serializes that mirror
  ([SRS-IN-09](../vector-document/srs-data.md)).

---

## [SRS-IN-17] Device debug-log channel (TCP :9878) {#srs-in-17-debug-log-channel}

Parent REQ: [REQ-03](../../prd.md#tablet-sync).
Peer: [SRS-EP-15](../../../epaper/features/device-document/srs-logic.md#srs-ep-15-debug-log-ship).
UI: [SRS-IN-18](./srs-ui.md#srs-in-18-device-log-panel). Quality: [SRS-IN-19](./srs-quality.md#srs-in-19-debug-log-isolation).

Sidecar **observability** for a live tablet session so a human can inspect device
console output (EP-012…016 enclose / ingest / sync) without touching the document
channel. **Not** a document channel. **Not** `doc_change`. **Not** Smart Group.
Does **not** amend [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md).

### Endpoint(s)

JSON-lines over TCP. Auth: none in v0 (trusted local link).

| Role | Bind / connect | Default |
|---|---|---|
| Infini | listen `0.0.0.0:9878` | `INFINI_DEBUG_PORT` (default **9878**) |
| Epaper | connect `{RM_SYNC_HOST}:9878` when env-gated | [SRS-EP-15](../../../epaper/features/device-document/srs-logic.md#srs-ep-15-debug-log-ship) |

Document session remains `INFINI_STROKE_PORT` / `:9877` ([SRS-IN-07](#srs-in-07)). **Two sockets,
two decoders.** Sharing a listen port, a parser, or a `type` switch with `:9877` is a spec defect.

### Closed ids (message types) — :9878 only

| id | Direction | Meaning |
|---|---|---|
| `debug_request` | Infini → Epaper | Desktop wants a debug session (panel opening or already open at connect) |
| `debug_start` | Infini → Epaper | Begin shipping `debug_log` |
| `debug_stop` | Infini → Epaper | Stop shipping; connection may stay up |
| `debug_log` | Epaper → Infini | One log record |

No other `type` values are legal on `:9878`. Handshake types from ADR-0015 (`viewport`,
`doc_load`, `doc_change`, `hello`, `stroke_*`, …) arriving here are **dropped** — never
forwarded to the `:9877` decoder, never applied to `VectorDocument`.

### Control sequence

```text
Infini listen :9878
Epaper (env on) connect
  |<-- debug_request ----------------|   (on connect, and when the panel opens)
  |<-- debug_start ------------------|   (panel open — shipping on)
  |-- debug_log × n ---------------->|
  |<-- debug_stop -------------------|   (panel closed — shipping off)
```

| Rule | Value |
|---|---|
| Shipping default | **Off** until `debug_start`. Idle TCP is allowed; idle is not a stream |
| `debug_request` | Does not start shipping. Device may no-op if already connected |
| `debug_start` while already shipping | Idempotent |
| `debug_stop` while already stopped | Idempotent |
| Panel closed | Send `debug_stop`. Infini **keeps** its in-memory buffer |
| Panel reopened | Send `debug_request` then `debug_start`; buffer still there |
| Device never connects | UI `disconnected` ([SRS-IN-18](./srs-ui.md#srs-in-18-device-log-panel)). **0** retries that touch `:9877` |
| Process restart | Buffer gone. Device env off → no connect ([SRS-EP-15](../../../epaper/features/device-document/srs-logic.md#srs-ep-15-debug-log-ship)) |

### `debug_log` envelope

```text
{ "type": "debug_log",
  "ts": <epoch_ms>,
  "level": "info" | "warning" | "critical" | "stdout" | "stderr",
  "logger": "qt" | "stdio",
  "msg": "<text>",
  "dropped": <int ≥ 0> }
```

| Field | Required | Meaning |
|---|---|---|
| `type` | yes | `debug_log` |
| `ts` | yes | Device clock, milliseconds since epoch (monotonic-enough for ordering in a session) |
| `level` | yes | Qt `qInfo` → `info`; `qWarning` → `warning`; `qCritical`/`qFatal` → `critical`; captured stdout → `stdout`; stderr → `stderr` |
| `logger` | yes | `qt` for Qt handler; `stdio` for fd capture |
| `msg` | yes | Log text, including any `[tag]` prefix (e.g. `[enclose]`). May be truncated at **4 KiB** |
| `dropped` | yes | Count of records discarded to backpressure **since the previous emitted** `debug_log` (0 if none) |

Unknown extra fields: keep, display as text if needed, **never** interpret as document ops.

### Isolation from the document (testable)

| Rule | Value |
|---|---|
| Apply to `VectorDocument` | **0** mutations from any `:9878` line |
| Mirror / save / preview | Debug records are **never** persisted, never preview strokes, never `doc_change` |
| Decoder | Dedicated debug decoder. The `:9877` `type` switch must not grow `debug_*` cases |
| Cross-port leak | A `debug_log` on `:9877` is an unknown type (protocol defect). A `doc_change` on `:9878` is dropped |

### Infini buffer

In-memory ring on the desktop process (Electron main or renderer — implementer's choice,
one owner).

| Rule | Value |
|---|---|
| Cap | **10_000** records. Overflow drops **oldest** |
| Lifetime | Process memory only. Panel close does **not** clear. App quit clears |
| Filter | Client-side only ([SRS-IN-18](./srs-ui.md#srs-in-18-device-log-panel)). Device is not asked to filter |
| Backpressure toward device | Infini never ACKs per line. Device drops under its own queue ([SRS-EP-15](../../../epaper/features/device-document/srs-logic.md#srs-ep-15-debug-log-ship)) |

### Errors / partial failure

| Case | Behavior |
|---|---|
| Bad JSON line on :9878 | Drop the line; keep the socket |
| Unknown `:9878` `type` | Drop; do not apply; do not send on `:9877` |
| Socket drop mid-stream | UI `disconnected`; buffer retained; on reconnect send `debug_request` and `debug_start` if the panel is still open |
| Browser-only (no Electron) | No listen; button still present; panel `disconnected` |

---

## [SRS-IN-13] Tool intent transport {#srs-in-13-tool-intent-transport}

<!-- lifecycle: retired -->
<!-- retired: 2026-08-13 — CHL-0008 / ADR-0014 -->
<!-- superseded-by: [SRS-EP-08], [SRS-EP-11] -->

> **RETIRED 2026-08-13** by [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) and
> [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md). Nothing inherits this section: the
> device no longer *asks* what it may touch, it acts on its own document. `stroke_begin.intent`,
> `doc_snapshot.pickables`, `tool_intent`, and the advisory-ghost authority rule are all gone from
> the wire. The device-side successors are
> [SRS-EP-11](../../../epaper/features/ink-box/srs-logic.md) (local hit-test and manipulation) and
> [SRS-EP-08](../../../epaper/features/device-document/srs-logic.md) (the sync contract).
>
> Text preserved below as the record of what was withdrawn — the ghost model in "Authority" is the
> precise thing [CHL-0006](../../../../../.plan/iter-003/challenges/CHL-0006-live-direct-resize.md)
> and [CHL-0007](../../../../../.plan/iter-003/challenges/CHL-0007-selection-move-enclose-sync.md)
> failed on.

**Parent:** [REQ-03](../../prd.md#tablet-sync) · [REQ-04](../../prd.md#smart-group).
**ADR:** [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md).

Carries **what the creator meant** from Epaper to Infini without putting tool state, a document
tree, or geometry recognition on the device. Tool mode itself is never transmitted (ADR-0013 §1).

### Stroke intent (Epaper → Infini) — additive field

| Field | Required | Meaning |
|---|---|---|
| `stroke_begin.intent` | no | `ink` (default) \| `enclose` |

Absent or unrecognised → `ink`, which is today's behaviour, so an older device stays correct.
The stroke still streams and paints as ordinary ink; `intent` only decides whether
[SRS-IN-10](../vector-document/srs-logic.md) evaluates it at `stroke_end`.

### Pickables (Infini → Epaper) — additive `doc_snapshot` array

```text
{ "type": "doc_snapshot",
  "nodes":     [ WorldLayerNode, … ],          # unchanged
  "pickables": [ { "id", "kind", "bounds" } ]  # new
}
```

| Field | Meaning |
|---|---|
| `id` | Node id, stable across snapshots |
| `kind` | `smart_group` only in the pilot |
| `bounds` | World AABB **after** transform — what the device hit-tests and ghosts |

Sent with every `doc_snapshot`. Devices that ignore the array keep working (picking is simply
unavailable there).

### Tool intent (Epaper → Infini)

```text
{ "type": "tool_intent", "action": "select" | "move" | "resize",
  "nodeId": …, "delta": { "dx", "dy" }?, "bounds": { … }?, "seq": n }
```

| Rule | Value |
|---|---|
| Emission | One message per completed gesture (on pen-up), never per sample |
| Authority | Infini applies the op; the device's ghost is **advisory** and discarded on the next `doc_snapshot` |
| Unknown `nodeId` | Log and ignore — the device may be acting on a stale snapshot |
| Stale `seq` | Last write wins; no locking in v0 |
| In-flight stroke | Queued behind an active Epaper stroke, same rule as structure ops (`setEpaperStrokeInFlight`) |

`tool_intent` is a **pilot-scoped** channel. It retires when the ADR-0009 op-log migration lands
and `doc_op` carries these edits in both directions.

### Errors / partial failure (SRS-IN-13)

| Case | Behavior |
|---|---|
| `intent: enclose` but guards fail | Stroke stays ink; no message back; device sees no change (correct) |
| `tool_intent` for a node deleted meanwhile | Ignore; next snapshot re-syncs the device |
| Snapshot arrives mid-ghost | Ghost discarded; authoritative geometry wins, even if it "jumps" |
| Device never receives `pickables` | Selection tool inert on device; pen and ink-box unaffected |

---

## Superseded

W5 wording that required live `doc_op` / `append_ink` / PNG `region_refresh` as the
production path is **superseded** by this 0.4.0 code-truth rewrite (2026-08-11). Target
op-log remains ADR-0009 long-term.
