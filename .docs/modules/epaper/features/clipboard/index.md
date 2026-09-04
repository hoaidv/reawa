---
feature: clipboard
parent_req: [REQ-12]
lifecycle: active
module: epaper
needs_design: false
---

# Feature — In-document clipboard (Epaper)

Copy, cut, and paste a cluster that is already right, **on the tablet**, without redrawing and
without the OS pasteboard. Chrome is frozen in SRS (no design story).

- Product REQ: [REQ-12 copy, cut, and paste](../../prd.md#clipboard)
- Product depth: [srs-product.md](./srs-product.md) — PM-owned consolidation
- Logic: [device-document SRS-EP-31](../device-document/srs-logic.md#srs-ep-31-clipboard)
- UI: [ink-box SRS-EP-32](../ink-box/srs-ui.md#srs-ep-32-clipboard-ui)
- Quality: [device-document SRS-EP-33](../device-document/srs-quality.md#srs-ep-33-clipboard-quality)
- Hold-still routing: [SRS-EP-11](../ink-box/srs-logic.md#srs-ep-11-hold-still) tap vs travel (no 500 ms menu)
- BDD: [device-document bdd/clipboard.feature](../device-document/bdd/clipboard.feature)
- Decision: [ADR-0037](../../../../adr/ADR-0037-device-clipboard-singleton.md) (supersedes [ADR-0024](../../../../adr/ADR-0024-in-document-clipboard.md))
- Siblings: [device-document](../device-document/index.md) (tree + undo) · [ink-box](../ink-box/index.md) (selection chrome)
