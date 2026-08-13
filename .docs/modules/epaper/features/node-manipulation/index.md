---
feature: node-manipulation
parent_req: [REQ-08]
lifecycle: active
module: epaper
needs_design: true
campaign: next
---

# Feature — Direct manipulation of any document node

One manipulation vocabulary for the whole document. Learning to move a Smart Group should teach the
creator how to move text, a primitive, a frame, or a connector — with per-kind tools layered on top
of a shared base, declared through a **capability descriptor** rather than hard-coded per kind.

**Status: thickened, not built.** [REQ-08](../../prd.md#node-manipulation) is a **distinct
iteration** after the [REQ-04](../../prd.md#device-document)…[REQ-07](../../prd.md#one-way-sync)
wave is verified. [ink-box](../ink-box/index.md) ([REQ-06](../../prd.md#device-manipulation)) ships
first and must be a **conforming subset** — the model here is written now precisely so that it is.

- Product REQ: [REQ-08 direct manipulation of any document node](../../prd.md#node-manipulation)
- Product depth (**the thickening**): [srs-product.md](./srs-product.md) — PM-owned
- Experience: [srs-experience.md](./srs-experience.md) — journeys
- Logic / UI / Quality: **deferred to the REQ-08 iteration** (architect + designer)
- Decision: **ADR-0016** (deferred) — manipulation model + capability descriptor
- First conforming citizen: [ink-box](../ink-box/srs-product.md) — BR-B17, BR-B18
- Node kinds it must cover: [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) ·
  [ADR-0011](../../../../adr/ADR-0011-smart-group.md)
- Human review gate: the model in `srs-product.md` requires explicit human approval before the
  architect designs against it
