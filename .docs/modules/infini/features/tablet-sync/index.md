---
feature: tablet-sync
parent_req: [REQ-03]
lifecycle: active
module: infini
---

# Feature — Tablet sync (Infini side)

Session with Epaper: ingest `stroke_*`; publish `viewport` + `doc_snapshot`.

- Product: [PRD REQ-03](../../prd.md#tablet-sync)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-07]
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-08]
- BDD: [bdd/session-channels.feature](./bdd/session-channels.feature)
- Document model: [vector-document](../vector-document/index.md)
- Sibling: [epaper/region-sync](../../../epaper/features/region-sync/index.md)
- Protocol: [epaper/protocol/viewport-sync.md](../../../../epaper/protocol/viewport-sync.md)
- Decision: [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) · [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)
