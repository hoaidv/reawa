---
id: STORY-EP-047
title: Recognize and preserve endpoint ink
kind: implement
parent_srs: [SRS-EP-74, SRS-EP-35, SRS-EP-37, SRS-EP-10]
parent_req: [REQ-13]
status: done
priority: P1
iter: iter-005
estimate: 5
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a tick that also lies inside the bound box, When pen-up runs, Then endpoint-ink wins over draw-into membership."
  - "Given Connector recognition armed and an existing connector, When a stroke has >=80% length in one end's 5 mm circle, Then it is bound on that end's ConnectorAnchor (0 free Ink; 0 second connector) and paints as drawn."
  - "Given decoration on an end, When the bound node rotates or the peer moves, Then stored {n, e} and drawn leave stay unchanged and world paint follows the re-warped leaving tangent (0 orphaned samples)."
  - "Given a second stolen stroke on the same end, When it commits, Then it appends; one undo removes only that stroke."
  - "Given the same stroke over empty canvas or the connector spine, or Connector recognition off, When the stroke ends, Then it is not stolen as endpoint style."
  - "Given a wrong endpoint-ink bind, When the creator undoes once, Then the document matches pre-stroke."
  - "Given decoration on an end, When the creator brush- or object-erases those ticks, Then the connector remains."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-047 — Recognize and preserve endpoint ink

TRACK-005. Parent [REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends) Path B. Product: [SRS-EP-74](../../../.docs/modules/epaper/features/connector-ink/srs-product.md#srs-ep-74-endpoint-ink-product). Logic: [SRS-EP-35](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-35-endpoint-ink). Decision: [ADR-0038](../../../.docs/adr/ADR-0038-endpoint-ink-face-frame.md). **No** design `depends_on`. Host tests in `epaper/tests/endpoint_ink_test.cpp`. **Human-verified on device 2026-09-05.**



## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

