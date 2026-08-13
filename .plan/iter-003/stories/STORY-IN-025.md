---
id: STORY-IN-025
title: Pickables inkScaleMode + members
kind: implement
parent_srs: [SRS-IN-13]
parent_req: [REQ-03]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — pickables transport retires with SRS-IN-13. Not scheduled; SM re-slices."
priority: P1
iter: iter-003
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a Smart Group in the doc, when buildPickables runs, then each pickable includes inkScaleMode and members[{id,role}] for child ink path ids"
  - "Given doc_snapshot to RM, when pickables are sent, then additive fields are present without breaking older devices"
---

# STORY-IN-025 — Pickables inkScaleMode + members

[SRS-IN-13](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) · CHL-0005

## Done when

- `buildPickables` emits `inkScaleMode` + `members`
- Unit coverage for member roles — green
