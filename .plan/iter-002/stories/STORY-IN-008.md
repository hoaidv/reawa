---
id: STORY-IN-008
title: "SVG persistence and shared op JSON fixtures"
kind: implement
parent_srs: [SRS-IN-09]
parent_req: [REQ-02]
status: done
priority: P1
iter: iter-002
estimate: 3
owner: dev
depends_on: [STORY-IN-007]
acceptance_criteria:
  - "Given a materialised tree, When save runs, Then an SVG profile round-trips load→tree without losing required ink sample channels (x,y + reported tablet extras)."
  - "Given unknown Infini-required structure on load, When parse runs, Then v0 fails closed; foreign SVG fluff may warn-and-skip."
  - "Given SRS-IN-09 schemas, When dual fixtures are authored, Then TS and Qt consumers share the same op envelope examples ({ opId, type, payload, source })."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-008 — SVG persistence and shared op JSON fixtures

Implements [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md).
Blocked on [STORY-IN-007](./STORY-IN-007.md). Addresses architect concern: dual TS+Qt fixtures
before sync ship.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-007 |

## Done when

- AC green under BDD `@SRS-IN-09`
- Shared fixtures reachable from Infini + Epaper test paths
