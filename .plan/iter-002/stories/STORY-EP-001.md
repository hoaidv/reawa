---
id: STORY-EP-001
title: "Epaper region sync map append_ink and refresh"
kind: implement
parent_srs: [SRS-EP-02]
parent_req: [REQ-02]
status: done
priority: P1
iter: iter-002
estimate: 5
owner: dev
depends_on: [STORY-IN-009]
acceptance_criteria:
  - "Given a viewport message from Infini, When Epaper receives it, Then input→world map updates before the next pen sample and a region refresh is enqueued."
  - "Given local pen ink, When a stroke completes, Then Epaper emits append_ink in world space with full tablet channels and does not run on-device Smart Group enclose."
  - "Given a remote doc_op, When Epaper applies it by opId, Then the next region refresh paints document ∩ drawing region without mixing stale map and stale doc in one pass."
  - "Given append_ink send failure, When retry/backoff runs, Then the local ink hot path is not blocked."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-001 — Epaper region sync map append_ink and refresh

Implements [SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md)
(+ [SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md) in verify).
Blocked on Infini session bind [STORY-IN-009](./STORY-IN-009.md). Sibling of tablet-sync.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-009 |

## Done when

- AC green under BDD `@SRS-EP-02`
- Hardware or fixture path exercises map + append_ink + refresh
