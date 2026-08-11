---
feature: vector-document
parent_req: [REQ-02, REQ-04]
lifecycle: active
module: infini
---

# Feature — Vector document

Tree-of-vectors document: ink, text, primitives, groups, frames, connectors; SVG persistence;
in-memory tree; transmit op-log. See [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md).

- Product REQ: [PRD REQ-02](../../prd.md#vector-document)
- Experience: [srs-experience.md](./srs-experience.md)
- Product depth: [srs-product.md](./srs-product.md)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-04] tree · [SRS-IN-10] enclose (tool-armed) ·
  [SRS-IN-11] selection/manipulation · [SRS-IN-12] undo · [SRS-IN-15] draw-into ·
  [SRS-IN-16] selection-create surround
- UI: [srs-ui.md](./srs-ui.md) — [SRS-IN-05] doc chrome · [SRS-IN-14] ink-box tools + overlay
- Data: [srs-data.md](./srs-data.md) — [SRS-IN-09]
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-06]
- BDD: [bdd/tree-ops.feature](./bdd/tree-ops.feature), [bdd/persistence.feature](./bdd/persistence.feature)
- Fixtures: [fixtures/ops/](./fixtures/ops/) (shared TS + Qt op envelopes)
- ADRs: [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md), [ADR-0011](../../../../adr/ADR-0011-smart-group.md),
  [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md)
- Pilot REQ: [REQ-04 Smart Group](../../prd.md#smart-group)
- Device peer: [epaper/tool-modes](../../../epaper/features/tool-modes/index.md)
