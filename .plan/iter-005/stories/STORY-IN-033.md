---
id: STORY-IN-033
title: Infini applies tablet viewport only while following
kind: implement
parent_srs: [SRS-IN-20, SRS-IN-21, SRS-IN-22, SRS-IN-26]
parent_req: [REQ-03, REQ-06]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-039, STORY-IN-037]
acceptance_criteria:
  - "Given Infini follow is on and a tablet two-finger pan/pinch, When the viewport message arrives, Then Infini canvas matches after settle."
  - "Given Infini follow is off, When the tablet pans, Then Infini's camera is unchanged (0 apply)."
  - "Given Infini is following, When the Infini user pans locally, Then follow turns off and Epaper's camera is unchanged."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-033 — Infini applies tablet viewport only while following

TRACK-005. Peer of [STORY-EP-039](./STORY-EP-039.md). Infini [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) + [REQ-03](../../../.docs/modules/infini/prd.md#tablet-sync). Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) is **superseded** by [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md).



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-039, STORY-IN-037 |

Independent cameras + optional follow bound 2026-08-20.
