---
id: STORY-EP-053
title: Design Epaper viewport-follow Infini toggle
kind: design
parent_srs: [SRS-EP-50, SRS-EP-49, SRS-EP-51]
parent_req: [REQ-19]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given REQ-19 states, When the package ships, Then one scene HTML exists per listed journey and ui-spec-gate passes."
  - "Given the follow control, When shown, Then it is an icon toggle — not a ToolChip exclusive tile."
  - "Given Infini is following Epaper, When Epaper chrome is shown, Then the Epaper toggle is off/disabled (exactly one direction)."
design_package: ".plan/iter-005/design/viewport-follow-epaper/"
ui_spec: ".plan/iter-005/design/viewport-follow-epaper/ui-spec.md"
scenes:
  - ".plan/iter-005/design/viewport-follow-epaper/viewport-follow-epaper-off.html"
  - ".plan/iter-005/design/viewport-follow-epaper/viewport-follow-epaper-following-infini.html"
  - ".plan/iter-005/design/viewport-follow-epaper/viewport-follow-epaper-peer-following-you.html"
  - ".plan/iter-005/design/viewport-follow-epaper/viewport-follow-epaper-connection-lost.html"
  - ".plan/iter-005/design/viewport-follow-epaper/viewport-follow-epaper-reconnect-stays-off.html"
hifi: ".plan/iter-005/design/viewport-follow-epaper/viewport-follow-epaper-off.html"
wireframe: ""
---

# STORY-EP-053 — Design Epaper viewport-follow Infini toggle

TRACK-005. Parent [REQ-19](../../../.docs/modules/epaper/prd.md#viewport-follow). [SRS-EP-50](../../../.docs/modules/epaper/features/region-sync/srs-ui.md#srs-ep-50-follow-toggle). [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md).

Package `viewport-follow-epaper/`. States: off (default); following Infini; peer-following-you (cannot enable); connection lost → off; reconnect stays off. **Do not** bury in [STORY-EP-037](./STORY-EP-037.md).

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |
