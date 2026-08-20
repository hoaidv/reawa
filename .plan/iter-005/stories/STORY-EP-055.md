---
id: STORY-EP-055
title: Epaper follow Infini — toggle, exclusion, disconnect
kind: implement
parent_srs: [SRS-EP-49, SRS-EP-51]
parent_req: [REQ-19]
status: done
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-053]
acceptance_criteria:
  - "Given a live session and both follows off, When the creator turns Epaper follow on, Then Infini follow is off and Epaper applies Infini viewport (p95 <=300 ms)."
  - "Given Infini follow is on, When the creator taps the Epaper follow toggle on, Then Infini follow turns off and Epaper follow turns on (exactly one direction; tap takes over)."
  - "Given Epaper is following, When the connection is lost, Then follow is off and does not restore on reconnect."
  - "Given Epaper is following, When the creator pans locally on Epaper, Then follow turns off (follower local-nav)."
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

# STORY-EP-055 — Epaper follow Infini — toggle, exclusion, disconnect

TRACK-005. Parent [REQ-19](../../../.docs/modules/epaper/prd.md#viewport-follow). [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md). Depends on [STORY-EP-053](./STORY-EP-053.md). Behavior-driven scenarios: [viewport-follow-epaper.feature](../../../.docs/modules/epaper/features/region-sync/bdd/viewport-follow-epaper.feature). **Ready** for Developer.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-053 |
