---
feature: device-document
parent_req: [REQ-04, REQ-07, REQ-11, REQ-12, REQ-17]
lifecycle: active
module: epaper
---

# Feature — On-device working document (Epaper side)

The device holds the document while the creator works on it: it ingests its own ink into a tree,
paints the panel from that tree, keeps an undo history, and publishes changes to the desktop. The
desktop contributes exactly one document input — the initial full load.

Established 2026-08-13 by [CHL-0008](../../../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md);
inherits semantics from the deprecated infini sections listed in the
[lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).

- Product REQ: [REQ-04 on-device working document](../../prd.md#device-document) ·
  [REQ-07 one-way document sync](../../prd.md#one-way-sync)
- Product depth: [srs-product.md](./srs-product.md) — PM-owned
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-07](./srs-logic.md#srs-ep-07-device-document) device document, ingestion, undo ·
  [SRS-EP-08](./srs-logic.md#srs-ep-08-one-way-sync) one-way sync (load handshake, publish queue) ·
  [SRS-EP-15](./srs-logic.md#srs-ep-15-debug-log-ship) debug-log ship TCP `:9878` (not a document channel) ·
  [SRS-EP-28](./srs-logic.md#srs-ep-28-selection-erase) selection-erase ·
  [SRS-EP-31](./srs-logic.md#srs-ep-31-clipboard) clipboard ·
  [SRS-EP-45](./srs-logic.md#srs-ep-45-manual-insert) manual Frame/Primitive
- Data: [srs-data.md](./srs-data.md) — [SRS-EP-09] device structures + wire binding; the grammar
  itself stays canonical in [infini SRS-IN-09](../../../infini/features/vector-document/srs-data.md)
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-13] ingestion budget, publish latency,
  offline parity, round-trip fidelity · [SRS-EP-16] debug-log ink-path isolation ·
  [SRS-EP-33] clipboard fidelity
- BDD: [bdd/ingest-stroke.feature](./bdd/ingest-stroke.feature) ·
  [bdd/undo-ring.feature](./bdd/undo-ring.feature) ·
  [bdd/undo-fail-safe.feature](./bdd/undo-fail-safe.feature) ·
  [bdd/undo-queue.feature](./bdd/undo-queue.feature) ·
  [bdd/one-way-sync.feature](./bdd/one-way-sync.feature) ·
  [bdd/debug-log-ship.feature](./bdd/debug-log-ship.feature)
- Shared node semantics: [domain/vector-document](../../../../domain/vector-document.md) ·
  [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) ·
  [ADR-0011](../../../../adr/ADR-0011-smart-group.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) (ownership
  inversion) · [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) (one-way sync contract) ·
  [ADR-0024](../../../../adr/ADR-0024-in-document-clipboard.md) (clipboard slot)
- Peers: [infini/tablet-sync](../../../infini/features/tablet-sync/index.md) ·
  [infini/vector-document](../../../infini/features/vector-document/index.md) (mirror + persistence)
- Siblings: [ink-box](../ink-box/index.md) (what edits the document) ·
  [region-sync](../region-sync/index.md) (viewport map) ·
  [local-pen-ink](../local-pen-ink/index.md) (the latency floor this must not regress)
