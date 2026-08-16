---
id: STORY-IN-030
title: Mirror create_connector envelope and derived warp
kind: implement
parent_srs: [SRS-IN-09]
parent_req: [REQ-09]
status: done
priority: P0
iter: iter-004
estimate: 5
owner: dev
depends_on: [STORY-EP-030]
acceptance_criteria:
  - "Given create_connector from the device, When Infini applies it, Then from/to/warpStyle/body/restShape round-trip with 0 loss (SRS-IN-09)."
  - "Given the same rest shape + endpoints + style, When both ends warp, Then samples are byte-comparable (0 divergent nodes, REQ-07)."
  - "Given set_smart_transform on a bound box, When Infini applies it, Then it emits 0 connector ops and re-derives geometry."
  - "Given a bound node is missing, When Infini draws the connector, Then it uses last live pose and does not mark invalid (D39)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-030 — Mirror create_connector envelope and derived warp

Infini is viewer + persistence: apply the op, derive the warp
([ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md)), do not author connectors
this campaign.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-030 |
| Parallel | **EP-033** (human 2026-08-16) |
