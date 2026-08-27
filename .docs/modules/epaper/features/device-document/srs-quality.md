---
feature: device-document
parent_req: [REQ-04, REQ-07]
version: 0.3.0
lifecycle: active
---

# SRS — On-device working document (Quality)

Parent REQs: [REQ-04](../../prd.md#device-document), [REQ-07](../../prd.md#one-way-sync).
Logic: [SRS-EP-07 / SRS-EP-08](./srs-logic.md). Desktop counterpart:
[SRS-IN-08](../../../infini/features/tablet-sync/srs-quality.md).

## [SRS-EP-13] Document, ingestion, undo, and sync budgets {#srs-ep-13-device-document-quality}

### The floor that outranks everything here

[SRS-EP-01](../local-pen-ink/srs-logic.md): **pen-down → pixel p95 ≤30 ms**. Every number below is
subordinate to it. If the document, the undo ring, or the publisher cannot fit under that ceiling,
the correct outcome is a `CHL-*` against this design — not a relaxed ink budget.

This is also the top risk in
[ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) Risks, so it is measured
**first**, before the first REQ-04 implement story ships.

### Document and ingestion

| Scenario | Metric | Target |
|---|---|---|
| Ink latency with the document in place | p95 pen-down → pixel | **≤30 ms** — equal to the pre-document baseline within measurement error |
| Stroke → node | p95 after pen-up | ≤50 ms |
| Ingestion vs ink contention | Samples dropped or delayed by ingestion | **0** |
| Repaint source | Repaints from an inbound peer picture | **0** ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2) |
| Document scale | 500 ink nodes / 50k samples resident | Ink budget still met; hit-test still ≤100 ms |
| Sample fidelity | Channels reported by the digitizer that survive into the node | **100%** |
| Carry-through | Node kinds the device does not author, preserved across a load → republish | **100%**, byte-identical |

### Undo

| Scenario | Metric | Target |
|---|---|---|
| Undo depth | Structural ops recoverable | **≥20** |
| Undo granularity | Ring entries per completed gesture | Exactly **1** |
| Undo exactness (rev match) | Tree after undo vs stored pre-op fields, when every target `lastOpId` equals the entry’s forward `opId` | Identical (0 divergent nodes; geometry ±1 world unit) |
| Undo skip (rev mismatch) | Later op’s fields after undo of an earlier op on the same node | **Unchanged**; 0 undo-through; whole entry skipped if any sibling changed |
| Undo no-op (absent) | Extra / half-inserted nodes; error UI | **0**; entry consumed |
| Undo latency | p95 request → panel shows the applied (or skip/no-op) state | ≤500 ms |
| Entry-size bound | Memory for a full ring at the 500-node fixture | Bounded and measured; **order-of-forward-op per entry**, not 20× document; shrink depth before slowing ink |
| Undo mid-gesture | Gestures corrupted by a deferred undo | **0** |
| Undo publish | `doc_change` for an applied undo | Counterpart or `compound`, **0** `restore_snapshot`; p95 commit → mirror ≤300 ms; payload order-of-forward-op |
| Undo vs ink budget | p95 pen-down → pixel with a full ring resident (500-node fixture) | **≤30 ms** — ring must not steal the ink budget |

<!-- lifecycle: retired -->
<!-- superseded-by: [ADR-0032] -->
<!-- note: 2026-08-27 — “tree after undo vs tree before the op / identical always” and Snapshot-cost (20 whole trees) retired. Exactness is lastOpId match → pre-op fields. -->

#### Quality-attribute scenarios (inverse ring)

| Source | Stimulus | Artifact | Environment | Response | Measure |
|---|---|---|---|---|---|
| Creator | Undo, target absent | Device tree | Live session | No-op those inverses; consume entry | 0 divergent extra nodes; 0 error UI |
| Creator | Undo, `lastOpId` mismatch | Device tree | Live or (future) second writer | Skip whole gesture | 0 undo-through; later op’s fields unchanged |
| Creator | Undo, rev matches | Device tree | Normal | Counterpart applied | 0 divergent nodes vs stored pre-op fields; geometry ±1 world unit |
| Creator | Undo mid-gesture | In-flight stroke | Normal | Latch; 0 corrupt gesture | **0** |
| Device | Undo publish | `doc_change` | Link up | Counterpart / `compound`, not wholesale tree | p95 commit → mirror ≤300 ms; payload order-of-forward-op |
| Device | Undo with document resident | Ink path | 500-node fixture | Ring must not steal the ink budget | p95 pen-down → pixel ≤30 ms |

### Sync — the one-way invariants

These are trace assertions on message type, not judgement calls
([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) Consequences).

| Scenario | Metric | Target |
|---|---|---|
| Inbound document messages after the initial load | Count over a full session trace | **0** |
| Unsolicited `doc_load` mid-session | Applied | **0** — rejected, logged, surfaced |
| `doc_load` offered while changes are queued | Queued changes discarded | **0** — queue drains first |
| Publish latency (link up) | p95 op commit → mirror updated | ≤300 ms |
| Change ordering | Ops applied out of `seq` order on the mirror | **0** |
| Change loss across a disconnect of ≥10 ops | Ops lost or reordered on reconnect | **0** |
| Duplicate delivery | Divergence after re-applying a duplicate `opId` | **0** (idempotent) |
| Handshake | Loads accepted before `queue_empty` (when `queued > 0`) | **0** |
| Preview hygiene | Preview strokes written to the mirror or saved | **0** |

### Offline parity

The product claim is that the link is irrelevant to editing
([srs-product](./srs-product.md) BR-D04), so it is measured as an equality, not a degradation.

| Scenario | Metric | Target |
|---|---|---|
| Scripted 10-gesture create + manipulate set, link down vs link up | Resulting documents | **Identical** (0 divergent nodes) |
| Features unavailable with the link down | Count | **0** |
| Queued changes visible to the creator | Pending state shown | Always ([SRS-EP-05](../tool-modes/srs-ui.md)) |
| Queue drain after a 10-minute disconnect | Ops published in order | 100%, `seq`-ordered |

### Round-trip fidelity

| Scenario | Metric | Target |
|---|---|---|
| Device-authored document → mirror → save → reopen → `doc_load` → device | `bounds`, `transform`, `inkScaleMode`, roles, `layoutOffset`, samples | Match within ±1 world unit @ 100% zoom |
| Shared fixture corpus (`ops/`, `enclose/`, `fixed-ink/`, `round-trip/`) | Device vs desktop verdicts | **100% agreement** — any divergence is a `CHL-*` |

### Failure behavior

| Case | Bar |
|---|---|
| Ingestion failure | Ink stays painted; document unchanged; **0** half-inserted nodes |
| Rejected op (invariant violation) | **0** published, **0** ring entries pushed |
| Load failure (malformed) | Current document intact; no `load_ack`; state visible |
| Session drop mid-gesture | Gesture completes and commits; **0** lost local work |
| App restart | Unpublished work lost — accepted; everything previously published returns on the next load |

### Notes

- "0" targets here are the ones worth arguing about: they encode that the failure modes of
  CHL-0004…CHL-0007 must be **impossible**, not merely rare. A flaky 0 is a defect in the design,
  not in the test.
- Instrumentation belongs at the transport (message type counts) and at op commit (latency), not in
  the paint loop — measurement must not become the thing that breaks the ink budget.

---

## [SRS-EP-16] Debug-log ship: ink-path isolation {#srs-ep-16-debug-log-ship-quality}

Parent REQ: [REQ-07](../../prd.md#one-way-sync).
Constrains: [SRS-EP-15](./srs-logic.md#srs-ep-15-debug-log-ship).
Desktop peer: [SRS-IN-19](../../../infini/features/tablet-sync/srs-quality.md#srs-in-19-debug-log-isolation).

Subordinate to [SRS-EP-01](../local-pen-ink/srs-logic.md) **p95 ≤30 ms** and to
[SRS-EP-13](#srs-ep-13-device-document-quality) ingestion contention **0**. If shipping
logs cannot fit under those ceilings, the shipper changes — not the ink budget.

| Field | Value |
|---|---|
| Source | Digitizer sample / `ingestPoint` / panel paint |
| Stimulus | Debug shipping on; Qt + stdio producing ≥200 lines/s; worker socket slow or blocked |
| Artifact | GUI/render/ink thread |
| Environment | Peak (enclose + ingest + live stroke) |
| Response | Sample still paints; enqueue or drop; worker owns the socket |
| Response measure | **0** log I/O (socket `write`/`flush`, blocking mutex, fd redirect read) on paint and on `ingestPoint`; pen-down → pixel still p95 ≤30 ms vs env-off baseline |

| Scenario | Metric | Target |
|---|---|---|
| Paint / `ingestPoint` | Debug-port or handler I/O on that stack | **0** |
| Backpressure | Paint stall / dropped pen samples caused by the shipper | **0**; oldest log records dropped |
| Device ship queue | Cap | **512**; overflow drops oldest |
| `debug_log` applied to `DeviceDocument` | Mutations / queued `doc_change` | **0** |
| `debug_*` on `:9877` | Sent by this feature | **0** |
| Env off | Connects to `:9878` | **0** |
| Stdio capture fail | Process abort | **0** — Qt logs still ship; one notice line |
| `[ink]` / `[enclose]` source | Lines per pen-up | Pen: **≤1** `[ink]`; ink_box: **≤1** `[enclose]`; **0** `[enclose]` when not ink_box; recognizer verdict unchanged (shared enclose fixtures still 100%) |

---

## [SRS-EP-33] Clipboard fidelity {#srs-ep-33-clipboard-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-12](../../prd.md#clipboard). **Constrains:** [SRS-EP-31](./srs-logic.md#srs-ep-31-clipboard). Does **not** steal [SRS-EP-13](#srs-ep-13-device-document-quality) parents (REQ-04/07).

| Scenario | Target |
|---|---|
| Copy then paste | New subtree, new ids; geometry = source + **(24 u, 24 u)** ±1 px @ 100% zoom; source unchanged |
| Cut then paste | Originals gone after cut; paste matches cut content ±1 px; undo paste removes copies; second undo restores originals |
| Empty slot paste | **0** nodes change |
| No session | Same local result; published ops satisfy REQ-07 when linked |

---

## Superseded

SRS-EP-13: new section. Replaces, for the device, the round-trip-shaped budgets formerly in
[SRS-EP-06](../tool-modes/srs-quality.md) and the snapshot-parity row of
[SRS-EP-03](../region-sync/srs-quality.md).

SRS-EP-16: additive — debug sidecar isolation; does not supersede SRS-EP-13.
SRS-EP-33: additive — clipboard; does not supersede SRS-EP-13.
