---
feature: tablet-sync
parent_req: [REQ-03]
lifecycle: active
module: infini
---

# Feature — Tablet sync (Infini side)

Session with Epaper under the one-way contract: publish `viewport` and exactly one `doc_load` per
epoch; apply inbound `doc_change`; render inbound `stroke_*` as transient preview.

- Product: [PRD REQ-03](../../prd.md#tablet-sync)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-07] · [SRS-IN-13] **retired**
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-08]
- BDD: [bdd/session-channels.feature](./bdd/session-channels.feature) ·
  [bdd/tool-intent-transport.feature](./bdd/tool-intent-transport.feature) (**retired**, kept for
  history)
- Document model: [vector-document](../vector-document/index.md) (mirror)
- Device counterpart: [epaper/device-document](../../../epaper/features/device-document/index.md) —
  [SRS-EP-08]
- Sibling: [epaper/region-sync](../../../epaper/features/region-sync/index.md)
- Protocol: [epaper/protocol/viewport-sync.md](../../../../epaper/protocol/viewport-sync.md)
- Decisions: [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) ·
  [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) (amended) ·
  [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)
