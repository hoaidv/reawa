---
feature: tablet-sync
parent_req: [REQ-03, REQ-05, REQ-06]
lifecycle: active
module: infini
---

# Feature — Tablet sync (Infini side)

Session with Epaper under the one-way **document** contract: publish exactly one `doc_load` per
epoch; apply inbound `doc_change`; render inbound `stroke_*` as transient preview. Viewport is
**follow-gated** ([ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md)).

- Product: [PRD REQ-03](../../prd.md#tablet-sync) · [REQ-06](../../prd.md#viewport-follow)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-07] · [SRS-IN-17] debug-log `:9878` · [SRS-IN-13] **retired** ·
  [SRS-IN-21] viewport emit/apply gates · [SRS-IN-26] follow Epaper · [SRS-IN-23] pen-button map persist/restore ([REQ-05](../../prd.md#pen-button-map)) — **not** an editor
- UI: [srs-ui.md](./srs-ui.md) — [SRS-IN-18] Device Log overlay (`needs_design: false`) ·
  [SRS-IN-24] pen-button map settings (**retired** — do not paint; editor is [SRS-EP-52](../../../epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor)) ·
  [SRS-IN-27] follow toggle (`needs_design: yes`) — **not** canvas chrome
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-08] · [SRS-IN-19] debug isolation · [SRS-IN-25] map persist/restore · [SRS-IN-28] follow exclusivity
- BDD: [bdd/session-channels.feature](./bdd/session-channels.feature) ·
  [bdd/handshake-doc-load.feature](./bdd/handshake-doc-load.feature) (STORY-IN-028) ·
  [bdd/device-log.feature](./bdd/device-log.feature) ·
  [bdd/tool-intent-transport.feature](./bdd/tool-intent-transport.feature) (**retired**, kept for
  history)
- Document model: [vector-document](../vector-document/index.md) (mirror)
- Device counterpart: [epaper/device-document](../../../epaper/features/device-document/index.md) —
  [SRS-EP-08] · [SRS-EP-15] debug-log ship
- Sibling: [epaper/region-sync](../../../epaper/features/region-sync/index.md)
- Anatomy: [domain/viewport-follow](../../../../domain/viewport-follow.md) · [domain/pen-button-map](../../../../domain/pen-button-map.md)
- Protocol: [epaper/protocol/viewport-sync.md](../../../../epaper/protocol/viewport-sync.md)
- Decisions: [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) ·
  [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) ·
  [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md) (amended) ·
  [ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md) ·
  [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md) (supersedes [ADR-0023](../../../../adr/ADR-0023-viewport-last-writer.md)) ·
  [ADR-0030](../../../../adr/ADR-0030-tablet-authors-pen-button-map.md) (supersedes [ADR-0028](../../../../adr/ADR-0028-pen-button-map-settings-channel.md))
  — debug family is **not** in ADR-0015; it lives on TCP `:9878` ([SRS-IN-17](./srs-logic.md#srs-in-17-debug-log-channel))
