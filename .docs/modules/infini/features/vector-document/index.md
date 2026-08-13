---
feature: vector-document
parent_req: [REQ-02, REQ-04]
lifecycle: active
module: infini
---

# Feature — Vector document

Tree-of-vectors document: ink, text, primitives, groups, frames, connectors; SVG persistence;
in-memory tree; inbound change applier. See [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md).

Since 2026-08-13 this tree is a **mirror**, not the session's source of truth — the device authors,
the desktop applies, paints, and saves
([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md)). The authoring sections below
are deprecated and re-homed to [epaper/ink-box](../../../epaper/features/ink-box/index.md).

- Product REQ: [PRD REQ-02](../../prd.md#vector-document)
- Experience: [srs-experience.md](./srs-experience.md)
- Product depth: [srs-product.md](./srs-product.md)
- Logic: [srs-logic.md](./srs-logic.md) — [SRS-IN-04] tree (**active**) ·
  [SRS-IN-10] enclose · [SRS-IN-11] selection/manipulation · [SRS-IN-12] undo ·
  [SRS-IN-15] draw-into · [SRS-IN-16] selection-create surround (**all deprecated**)
- UI: [srs-ui.md](./srs-ui.md) — [SRS-IN-05] doc chrome (**active**) ·
  [SRS-IN-14] ink-box tools + overlay (**deprecated**)
- Data: [srs-data.md](./srs-data.md) — [SRS-IN-09]
- Quality: [srs-quality.md](./srs-quality.md) — [SRS-IN-06]
- BDD: [bdd/tree-ops.feature](./bdd/tree-ops.feature),
  [bdd/persistence.feature](./bdd/persistence.feature),
  [bdd/tree-backed-ink.feature](./bdd/tree-backed-ink.feature) (mirror path).
  Smart Group features moved to [epaper/ink-box/bdd](../../../epaper/features/ink-box/bdd/) and
  [epaper/device-document/bdd](../../../epaper/features/device-document/bdd/)
- Fixtures: [fixtures/ops/](./fixtures/ops/) (shared TS + Qt op envelopes — now the divergence guard
  between two implementations)
- ADRs: [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md), [ADR-0011](../../../../adr/ADR-0011-smart-group.md),
  [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md),
  [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md)
- Shared semantics: [domain/vector-document](../../../../domain/vector-document.md)
- Deprecated REQ: [REQ-04 Smart Group](../../prd.md#smart-group)
- Device peer: [epaper/device-document](../../../epaper/features/device-document/index.md)
