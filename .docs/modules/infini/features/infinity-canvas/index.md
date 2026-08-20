---
feature: infinity-canvas
parent_req: [REQ-01]
lifecycle: active
module: infini
---

# Feature — Infinity canvas

Desktop infinity surface: pan, zoom/pinch, transform = translate + uniform scale.

- Product: [PRD REQ-01](../../prd.md#infinity-canvas) · follow toggle: [REQ-06](../../prd.md#viewport-follow) (not this package)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-01] · [SRS-IN-20] apply tablet viewport **while Infini following** (gate, not parent of REQ-06)
- UI: [srs-ui.md](./srs-ui.md) — [SRS-IN-02] (follow chrome is [SRS-IN-27](../tablet-sync/srs-ui.md))
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-03] · [SRS-IN-22] apply-while-following
- Decisions: [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md) (supersedes [ADR-0023](../../../../adr/ADR-0023-viewport-last-writer.md))
