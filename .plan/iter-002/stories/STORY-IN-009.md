---
id: STORY-IN-009
title: "Infini tablet session viewport and document channel"
kind: implement
parent_srs: [SRS-IN-07]
parent_req: [REQ-03]
status: done
priority: P1
iter: iter-002
estimate: 5
owner: dev
depends_on: [STORY-IN-007, STORY-IN-008]
acceptance_criteria:
  - "Given a live session, When Infini pans/zooms, Then a viewport message (translate, scale, drawingRegion, seq) is emitted on the viewport channel toward Epaper."
  - "Given an append_ink doc_op from Epaper, When Infini applies it, Then the tree updates idempotently by opId and WorldLayer reflects new ink."
  - "Given the v0 emit matrix, When Infini edits structure or Smart Group, Then those ops may emit on the document channel; Infini does not race an in-flight Epaper stroke."
  - "Given duplicate opId or unknown op type, When apply runs, Then duplicate is ignored and unknown is logged without crashing."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-009 — Infini tablet session viewport and document channel

Implements [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md)
(+ quality [SRS-IN-08](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md) budgets
in verify). Blocked on [STORY-IN-007](./STORY-IN-007.md), [STORY-IN-008](./STORY-IN-008.md).

**Note:** reconnect `snapshot`/`hello` remains TBD (architect READY-WITH-CONCERNS) — out of
this story's AC unless a CHL lands; degrade path may log only in v0.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-007, STORY-IN-008 |

## Done when

- AC green under BDD `@SRS-IN-07` (and latency checks tagged `@SRS-IN-08` as applicable)
