---
feature: region-sync
parent_req: [REQ-02]
lifecycle: active
module: epaper
---

# Feature — Region sync (Epaper side)

Apply Infini viewport to input map; emit stroke ops; refresh drawing region from shared document.

- Product: [PRD REQ-02](../../prd.md#region-sync)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-02]
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-03]
- BDD: [bdd/map-append-refresh.feature](./bdd/map-append-refresh.feature)
- Implementation: [epaper/regionsync/](../../../../epaper/regionsync/)
- Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/index.md)
- Local ink: [local-pen-ink](../local-pen-ink/index.md)
- Decision: [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)
