---
id: STORY-IN-037
title: Infini follow Epaper — toggle, exclusion, disconnect
kind: implement
parent_srs: [SRS-IN-26, SRS-IN-28]
parent_req: [REQ-06]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-IN-036]
acceptance_criteria:
  - "Given a live session and both follows off, When the creator turns Infini follow on, Then Epaper follow is off and Infini applies Epaper viewport (p95 <=300 ms)."
  - "Given Epaper follow is on, When the creator taps the Infini follow toggle on, Then Epaper follow turns off and Infini follow turns on (exactly one direction; tap takes over)."
  - "Given Infini is following, When the connection is lost, Then follow is off and does not restore on reconnect."
  - "Given Infini is following, When the creator pans Infini locally, Then follow turns off (follower local-nav)."
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

# STORY-IN-037 — Infini follow Epaper — toggle, exclusion, disconnect

TRACK-005. Parent Infini [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow). [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md). Depends on [STORY-IN-036](./STORY-IN-036.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-036 |
