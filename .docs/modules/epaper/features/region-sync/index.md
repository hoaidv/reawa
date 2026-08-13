---
feature: region-sync
parent_req: [REQ-02]
lifecycle: active
module: epaper
---

# Feature — Region sync (Epaper side)

Apply Infini `viewport`; emit `stroke_*` preview; gut-aware map; rasterize the **local document**.
The picture is no longer supplied by the peer
([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2).

- Product: [PRD REQ-02](../../prd.md#region-sync)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-02]
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-03]
- BDD: [bdd/map-append-refresh.feature](./bdd/map-append-refresh.feature)
- Runtime: `epaper/tabletcanvasitem.cpp` + `strokesync.cpp`
- Library (tests only): [epaper/regionsync/](../../../../epaper/regionsync/)
- Protocol: [epaper/protocol/viewport-sync.md](../../../../epaper/protocol/viewport-sync.md)
- What it paints: [device-document](../device-document/index.md) — [SRS-EP-07];
  document input handled by [SRS-EP-08]
- Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/index.md)
- Local ink: [local-pen-ink](../local-pen-ink/index.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) ·
  [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) (amended) ·
  [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)
