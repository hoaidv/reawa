---
feature: ink-box
parent_req: [REQ-05, REQ-06, REQ-10, REQ-12]
lifecycle: active
module: epaper
needs_design: true
---

# Feature — Ink-box on the device (create + manipulate)

Enclose a handwritten cluster and it becomes one object, **on the panel, immediately**. Select it,
move it, resize it, switch how its ink scales — all against the device's own document
([device-document](../device-document/index.md)), with no peer round trip and no advisory ghost.

Established 2026-08-13 by [CHL-0008](../../../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md).
Semantics inherited from the deprecated infini sections [SRS-IN-10], [SRS-IN-11], [SRS-IN-15],
[SRS-IN-16] — see the [lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).

- Product REQ: [REQ-05 on-device ink-box creation](../../prd.md#device-ink-box) ·
  [REQ-06 on-device ink-box manipulation](../../prd.md#device-manipulation)
- Experience (start here): [srs-experience.md](./srs-experience.md) — journeys
- Product depth: [srs-product.md](./srs-product.md) — PM-owned
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-EP-10] recognition, guards, membership ·
  [SRS-EP-11] selection, hit-test, live manipulation ·
  [SRS-EP-21] one-finger pick/move ([REQ-10](../../prd.md#hand-touch)) ·
  [SRS-EP-75] nested capture + flatten ·
  [SRS-EP-76] RenderingContext + content AABB clip ·
  [SRS-EP-77] nested tap + move reparent
- UI: [srs-ui.md](./srs-ui.md) — [SRS-EP-12] selection overlay ·
  [SRS-EP-22] hand-touch chrome · [SRS-EP-32] clipboard affordances (needs_design: no)
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-EP-14] create/manipulate bars ·
  [SRS-EP-25] one-finger hand-touch bars
- BDD: [bdd/](./bdd/) — inherited from the deprecated infini features and re-tagged
- Decisions: [ADR-0011](../../../../adr/ADR-0011-smart-group.md) (semantics survive) ·
  [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) (where they run) ·
  [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md) (empty pan is local; publish only if Infini following) ·
  [ADR-0039](../../../../adr/ADR-0039-nested-ink-box-rendering.md) (nested compose + own-transform)
- Forward constraint: must conform to [node-manipulation](../node-manipulation/srs-product.md)
  ([REQ-08](../../prd.md#node-manipulation))
- Architect view (lasso/move/resize routing): [tool-system](../../tool-system/index.md) ·
  [ADR-0033](../../../../adr/ADR-0033-tool-abstraction.md)
- Siblings: [tool-modes](../tool-modes/index.md) (how a tool is armed) ·
  [device-document](../device-document/index.md) (what it edits)
