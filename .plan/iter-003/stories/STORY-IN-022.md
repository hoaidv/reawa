---
id: STORY-IN-022
title: "Default fixedInk and side resize handles"
kind: implement
parent_srs: [SRS-IN-09]
parent_req: [REQ-04]
status: in-review
priority: P1
iter: iter-003
estimate: 2
owner: dev
depends_on: [STORY-IN-021]
acceptance_criteria:
  - "Given new Smart Group from enclose or surround create, When created, Then inkScaleMode is fixedInk."
  - "Given selected ink-box on desktop, When overlay shown, Then n/e/s/w edge handles visible in addition to corners."
design_package: ".plan/iter-003/design/ink-box-ui/"
ui_spec: ".plan/iter-003/design/ink-box-ui/ui-spec.md"
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-022 — Default fixedInk + side handles

Default `fixedInk` in enclose, surround, VectorDocument. Side handles in overlay CSS/DOM.
