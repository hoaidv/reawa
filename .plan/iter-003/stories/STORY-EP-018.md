---
id: STORY-EP-018
title: "On-device selection-create surround"
kind: implement
parent_srs: [SRS-EP-10, SRS-EP-11, SRS-EP-12, SRS-EP-14]
parent_req: [REQ-05]
status: draft
priority: P1
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-012, STORY-EP-016, STORY-EP-022]
acceptance_criteria:
  - "Given Selection tool and pen-down+move, When the gesture runs, Then ovl.marquee (thin dotted) follows the pen tip until pen-up."
  - "Given pen-up after marquee, When nodes intersect the rubber-band, Then ovl.nodes_bounds + 6 square anchors appear around those document nodes and cta.enclose is visible."
  - "Given ≥2 selected free inks where one stroke surrounds ≥80% of every other, When the user taps cta.enclose, Then the winner is role boundary, others are role content with layoutOffset UV, and bounds equal the winner AABB."
  - "Given an open surround stroke, When containment is tested, Then the artificial closed path (even-odd) may qualify and stored samples are unchanged."
  - "Given no qualifying surround or a SmartGroup in the selection, When cta.enclose is tapped, Then 0 boxes are created, selection is unchanged, and ind.create_refused_no_surround is visible."
  - "Given ToolChip, When Enclose is available, Then the chip still has exactly three tools (ADR-0016)."
design_package: ".plan/iter-003/design/selection-enclose-chrome/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-018 — On-device selection-create surround

Implements selection-create in [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md)
with chrome from [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) /
[ADR-0016](../../../.docs/adr/ADR-0016-selection-create-enclose-cta.md).
BDD: [selection-create-surround.feature](../../../.docs/modules/epaper/features/ink-box/bdd/selection-create-surround.feature)
(QA extends for marquee + Enclose CTA).

**Unfrozen for design.** Implement stays `draft` until [STORY-EP-022](./STORY-EP-022.md) is
`done` (ui_spec / scenes / hifi copied here). Do **not** invent chrome.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-012, EP-016, **EP-022 (design)** |

## Done when

- `@SRS-EP-10` selection-create scenarios green (incl. marquee + CTA)
- UI matches EP-022 package; ToolChip remains three tools
