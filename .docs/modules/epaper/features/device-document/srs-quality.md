---
feature: device-document
parent_req: [REQ-04, REQ-07]
version: 0.1.0
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
| Undo exactness | Tree after undo vs tree before the op | Identical (0 divergent nodes; geometry ±1 world unit) |
| Undo latency | p95 request → panel shows the restored state | ≤500 ms |
| Snapshot cost | Added memory for a full ring at the 500-node fixture | Bounded and measured; shrink the ring before slowing ink |
| Undo mid-gesture | Gestures corrupted by a deferred undo | **0** |

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

## Superseded

New section. Replaces, for the device, the round-trip-shaped budgets formerly in
[SRS-EP-06](../tool-modes/srs-quality.md) and the snapshot-parity row of
[SRS-EP-03](../region-sync/srs-quality.md).
