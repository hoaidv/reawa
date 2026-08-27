---
id: STORY-EP-060
title: Undo fail-safe skip and no-op catalogue
kind: implement
parent_srs: [SRS-EP-13, SRS-EP-07]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-059]
acceptance_criteria:
  - "Given a multi-node gesture where some targets are absent and none are changed, When undo runs, Then live targets apply, absences no-op, the entry is consumed, 0 error UI, 0 redo push if nothing applied except the live apply case (F21)."
  - "Given a multi-node gesture where any sibling lastOpId mismatches the forward op, When undo runs, Then the whole entry is skipped, later fields unchanged, the entry is consumed, 0 error UI, 0 redo push (F20)."
  - "Given an empty ring, When undo or redo is requested, Then it is a no-op (0 tree change, 0 error UI)."
  - "Given skip or pure no-op, When observed, Then 0 restore_snapshot is published."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-060 — Undo fail-safe skip and no-op catalogue

TRACK-005. Depends on [STORY-EP-059](./STORY-EP-059.md). [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md) skip/no-op rows. Binding: **F21** absences no-op; any **changed** sibling ⇒ skip whole (**F20**). v1 has no second author — mismatch is still the skip predicate.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-059 |

## Done when

- F20 / F21 fixtures green
- Skip/no-op consume the entry and do not push redo
- 0 error UI

Verified 2026-08-27: Quality Assurance Engineer **PASS**. Host map: `.docs/modules/epaper/features/device-document/bdd/undo-fail-safe.feature` (4 scenarios).
