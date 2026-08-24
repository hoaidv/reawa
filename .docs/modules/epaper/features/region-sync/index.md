---
feature: region-sync
parent_req: [REQ-02, REQ-10, REQ-19]
lifecycle: active
module: epaper
---

# Feature — Region sync (Epaper side)

Apply Infini `viewport` **only while Epaper follow is on**; emit `stroke_*` preview; gut-aware map; rasterize the **local document**. Default: independent camera. The picture is never supplied by the peer
([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2).

- Product: [PRD REQ-02](../../prd.md#region-sync) · [REQ-19](../../prd.md#viewport-follow) · two-finger [REQ-10](../../prd.md#hand-touch)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-02] inbound map (gated) · [SRS-EP-24] two-finger local + publish-if-followed · [SRS-EP-49] follow Infini
- UI: [srs-ui.md](./srs-ui.md) — [SRS-EP-50] follow toggle (`needs_design: yes`) — **not** ToolChip
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-03] · [SRS-EP-26] two-finger map-apply · [SRS-EP-51] follow exclusivity
- BDD: [bdd/map-append-refresh.feature](./bdd/map-append-refresh.feature)
- Runtime: `epaper/drawing/tabletcanvasitem.cpp` + `epaper/regionsync/strokesync.cpp`
- Library: [epaper/regionsync/](../../../../epaper/regionsync/)
- Protocol: [epaper/protocol/viewport-sync.md](../../../../epaper/protocol/viewport-sync.md)
- What it paints: [device-document](../device-document/index.md) — [SRS-EP-07];
  document input handled by [SRS-EP-08]
- Sibling: [infini/tablet-sync](../../../infini/features/tablet-sync/index.md)
- Local ink: [local-pen-ink](../local-pen-ink/index.md)
- Anatomy: [domain/viewport-follow](../../../../domain/viewport-follow.md)
- Decisions: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) ·
  [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) (amended) ·
  [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md) ·
  [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md) (supersedes [ADR-0023](../../../../adr/ADR-0023-viewport-last-writer.md))
