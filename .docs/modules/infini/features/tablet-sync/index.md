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
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-07] · [SRS-IN-17] debug-log `:9878` · [SRS-IN-13] **retired**
- UI: [srs-ui.md](./srs-ui.md) — [SRS-IN-18] Device Log overlay (`needs_design: false`)
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-08] · [SRS-IN-19] debug isolation
- BDD: [bdd/session-channels.feature](./bdd/session-channels.feature) ·
  [bdd/handshake-doc-load.feature](./bdd/handshake-doc-load.feature) (STORY-IN-028) ·
  [bdd/device-log.feature](./bdd/device-log.feature) ·
  [bdd/tool-intent-transport.feature](./bdd/tool-intent-transport.feature) (**retired**, kept for
  history)
- Document model: [vector-document](../vector-document/index.md) (mirror)
- Device counterpart: [epaper/device-document](../../../epaper/features/device-document/index.md) —
  [SRS-EP-08] · [SRS-EP-15] debug-log ship
- Sibling: [epaper/region-sync](../../../epaper/features/region-sync/index.md)
- Protocol: [epaper/protocol/viewport-sync.md](../../../../epaper/protocol/viewport-sync.md)
- Decisions: [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) ·
  [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) (amended) ·
  [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)
  — debug family is **not** in ADR-0015; it lives on TCP `:9878` ([SRS-IN-17](./srs-logic.md#srs-in-17-debug-log-channel))
