---
id: STORY-EP-058
title: Implement Device Settings page (Pen buttons)
kind: implement
parent_srs: [SRS-EP-52]
parent_req: [REQ-20]
status: draft
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-056]
acceptance_criteria:
  - "Given the leading 10 mm stylus-with-barrels tile, When tapped, Then Settings opens as a full-panel master-detail page with Pen buttons selected (p95 <=300 ms) and the exclusive tool is unchanged."
  - "Given Settings · Pen buttons, When shown, Then the master list contains only Pen buttons this package and Click / Hold-move catalogues are inline (0 sheets)."
  - "Given Settings · Pen buttons at 0-button / 1-button / 2-button / session-down, When shown, Then those are states of the same page and the live map still applies while session-down."
  - "Given the creator rebinds a present Click or Hold-move slot, When they dismiss Settings, Then the next barrel gesture uses the new binding and any in-flight gesture is unchanged."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ".plan/iter-005/design/pen-button-map/ui-spec.md"
scenes:
  - ".plan/iter-005/design/pen-button-map/pen-button-map-entry.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-0.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-layout-2.html"
  - ".plan/iter-005/design/pen-button-map/pen-button-map-offline.html"
hifi: ".plan/iter-005/design/pen-button-map/pen-button-map-layout-1.html"
wireframe: ""
---

# STORY-EP-058 — Implement Device Settings page (Pen buttons)

TRACK-005. Parent [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings). Painted by [STORY-EP-056](./STORY-EP-056.md) ([UI-EP-08](../design/pen-button-map/ui-spec.md)). One Settings page, master-detail, catalogues inline. [CHL-0025](../challenges/CHL-0025-pen-map-settings-page.md) adopted.

Barrel **dispatch** remains [STORY-EP-052](./STORY-EP-052.md) (REQ-18). On-device **persist** remains [STORY-EP-057](./STORY-EP-057.md).

Status stays **draft** until Solution Architect drops sheet scenes from SRS-EP-52 and behavior-driven scenarios exist.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-056 |
