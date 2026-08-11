---
id: STORY-IN-006
title: "Design Infini document open save chrome"
kind: design
parent_srs: [SRS-IN-05]
parent_req: [REQ-02]
status: ready
priority: P1
iter: iter-002
estimate: 2
owner: designer
depends_on: []
acceptance_criteria:
  - "Given states doc.none, doc.open, doc.dirty, doc.error, When the package ships, Then each has a scene or annotated state in ui-spec."
  - "Given minimal chrome only (SRS-IN-05), When hi-fi is produced, Then DocChrome (New/Open/Save, title, dirty) and DocError regions exist; no illustration-suite panels in v0."
  - "Given tree-of-vectors (ADR-0010), When populated scene is shown, Then WorldLayer may show ink + at least one group/frame/connector/text/primitive affordance or annotation so implementers see structure is not a flat stroke list."
  - "Given platform Electron desktop, When ui-spec records platform profile, Then data-platform=desktop and chrome regions are named for implement depends_on."
design_package: ".plan/iter-002/design/vector-document/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-006 — Design Infini document open save chrome

Design for [SRS-IN-05](../../../.docs/modules/infini/features/vector-document/srs-ui.md)
([REQ-02](../../../.docs/modules/infini/prd.md#vector-document),
[ADR-0010](../../../.docs/adr/ADR-0010-tree-of-vectors.md)).

**NOW** — F1 verified; SRS thickened 2026-08-11 (PM+Architect). Package:
`.plan/iter-002/design/vector-document/`.

Parent logic: [SRS-IN-04](../../../.docs/modules/infini/features/vector-document/srs-logic.md) ·
data [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md).
