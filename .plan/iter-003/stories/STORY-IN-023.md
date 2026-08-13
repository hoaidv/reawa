---
id: STORY-IN-023
title: fixedInk resize via scale+translate
kind: implement
parent_srs: [SRS-IN-11]
parent_req: [REQ-04]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — assumes desktop tree authority; behaviour re-specified under epaper REQ-06. Not scheduled; SM re-slices."
priority: P1
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a Smart Group in fixedInk mode, when the user resizes via handles, then transform.scaleX/Y and translate update so boundary ink stretches with the box"
  - "Given fixedInk resize, when mapping completes, then local bounds stay unchanged (UV sample size stable)"
  - "Unit: smartTransformFromWorldAabb(fixedInk) updates scale; local bounds fixed"
---

# STORY-IN-023 — fixedInk resize via scale+translate

[SRS-IN-11](../../../.docs/modules/infini/features/vector-document/srs-logic.md) · CHL-0004

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner persona | `dev` |

## Done when (implement)

- `smartTransformFromWorldAabb` uses scale+translate for both `fixedInk` and `withBounds`
- Desktop + tablet resize (via tool_intent) apply that mapping
- Unit tests green for fixedInk scale mapping
