---
feature: connector-ink
parent_req: [REQ-09, REQ-13, REQ-14, REQ-17]
lifecycle: active
owner: architect
---

# Feature: Connector-ink (on-device)

- **Parent Requirement:** [REQ-09 On-device connectors](../../prd.md#device-connectors)
- **Experience Spec:** [srs-experience.md](./srs-experience.md)
- **Product Spec:** [srs-product.md](./srs-product.md) — REQ-09 rules · [SRS-EP-74] Path B endpoint ink
- **UI Spec:** [srs-ui.md](./srs-ui.md) — [SRS-EP-19] · [SRS-EP-36] endpoint toolbar (**Infini / later — not Epaper this campaign**) · [SRS-EP-39] attachments
- **Logic Spec:** [srs-logic.md](./srs-logic.md) — [SRS-EP-17] recognition · [SRS-EP-18] warp ·
  [SRS-EP-34] end styles · [SRS-EP-35] endpoint-ink · [SRS-EP-38] attachment `t` · [SRS-EP-46] manual connector
- **Quality Spec:** [srs-quality.md](./srs-quality.md) — [SRS-EP-20] · [SRS-EP-37] ends · [SRS-EP-40] attachments
- **Decisions:** [ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md) geometry ·
  [ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md) ToolChip ·
  [ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md) dispatch ·
  [ADR-0038](../../../../adr/ADR-0038-endpoint-ink-face-frame.md) Path B endpoint-ink (supersedes [ADR-0026](../../../../adr/ADR-0026-endpoint-ink-membership.md)) ·
  [ADR-0027](../../../../adr/ADR-0027-attachment-t-rest-spine.md) attachment `t` ·
  [ADR-0036](../../../../adr/ADR-0036-toolcanvas-live-overlay.md) UX2 is last-3 free inks, not DFS
- **BDD:** [connector-recognition.feature](./bdd/connector-recognition.feature) ·
  [connector-warp.feature](./bdd/connector-warp.feature) ·
  [endpoint-ink.feature](./bdd/endpoint-ink.feature)
- **Brainstorm / EXP:** [BS-0001](../../../../.plan/iter-004/brainstorms/BS-0001-auto-connector-ink.md) ·
  [EXP-0002](../../../../.plan/iter-004/explorations/EXP-0002-connector-ink-warp.md)
- **Active iter:** [iter-005](../../../../.plan/iter-005/iter.md)
