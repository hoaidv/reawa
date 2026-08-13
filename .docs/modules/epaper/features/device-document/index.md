---
feature: device-document
parent_req: [REQ-04, REQ-07]
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
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-07] device document, ingestion, undo ·
  [SRS-EP-08] one-way sync (load handshake, publish queue)
- Data: [srs-data.md](./srs-data.md) — [SRS-EP-09] device structures + wire binding; the grammar
  itself stays canonical in [infini SRS-IN-09](../../../infini/features/vector-document/srs-data.md)
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-13] ingestion budget, publish latency,
  offline parity, round-trip fidelity
- Shared node semantics: [domain/vector-document](../../../../domain/vector-document.md) ·
  [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) ·
  [ADR-0011](../../../../adr/ADR-0011-smart-group.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) (ownership
  inversion) · [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) (one-way sync contract)
- Peers: [infini/tablet-sync](../../../infini/features/tablet-sync/index.md) ·
  [infini/vector-document](../../../infini/features/vector-document/index.md) (mirror + persistence)
- Siblings: [ink-box](../ink-box/index.md) (what edits the document) ·
  [region-sync](../region-sync/index.md) (viewport map) ·
  [local-pen-ink](../local-pen-ink/index.md) (the latency floor this must not regress)
