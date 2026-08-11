---
id: STORY-IN-006
title: "Design Infini document open save chrome"
kind: design
parent_srs: [SRS-IN-05]
parent_req: [REQ-02]
status: blocked
priority: P2
iter: iter-002
estimate: 2
owner: designer
depends_on: []
acceptance_criteria:
  - "CANCELLED — human 2026-08-11: no design package required for DocChrome in this wave."
design_package: ".plan/iter-002/design/vector-document/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-006 — Design Infini document open save chrome

## Status: blocked (cancelled)

**Human decision (2026-08-11):** STORY-IN-006 needs **no design**. Do **not** run `/designer`.
Implement of document chrome / tree / Smart Group is **deferred** until building **epaper ↔
desktop sync** (tablet-sync / region-sync wave) — **no `/dev` yet**.

SRS + ADRs remain the contract:

- [REQ-02](../../../.docs/modules/infini/prd.md#vector-document) · [REQ-04](../../../.docs/modules/infini/prd.md#smart-group)
- [ADR-0010](../../../.docs/adr/ADR-0010-tree-of-vectors.md) · [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md)
- Feature SRS under `.docs/modules/infini/features/vector-document/`

**Next persona:** `/architect` (sync readiness / bind document ops to tablet channel) — then
SM will slice implement stories when sync wave opens.
