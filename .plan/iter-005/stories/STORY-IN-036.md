---
id: STORY-IN-036
title: Design Infini viewport-follow Epaper toggle
kind: design
parent_srs: [SRS-IN-27, SRS-IN-26, SRS-IN-28]
parent_req: [REQ-06]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given Infini REQ-06 states, When the package ships, Then one scene HTML exists per listed journey and ui-spec-gate passes."
  - "Given the follow control, When shown, Then it is an icon toggle on the desktop — not document chrome (SRS-IN-05) and not the pen-button map (IN-034)."
  - "Given Epaper is following Infini, When Infini chrome is shown, Then the Infini toggle is off/disabled (exactly one direction)."
design_package: ".plan/iter-005/design/viewport-follow-infini/"
ui_spec: ".plan/iter-005/design/viewport-follow-infini/ui-spec.md"
scenes:
  - ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-off.html"
  - ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-following-epaper.html"
  - ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-peer-following-you.html"
  - ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-local-nav-turns-off.html"
  - ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-connection-lost.html"
  - ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-reconnect-stays-off.html"
hifi: ".plan/iter-005/design/viewport-follow-infini/viewport-follow-infini-off.html"
wireframe: ""
---

# STORY-IN-036 — Design Infini viewport-follow Epaper toggle

TRACK-005. Parent Infini [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow). [SRS-IN-27](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-27-follow-toggle). [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md).

Package `viewport-follow-infini/`. States: off; following Epaper; peer-following-you; connection lost → off; reconnect stays off. Platform **desktop**.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |
